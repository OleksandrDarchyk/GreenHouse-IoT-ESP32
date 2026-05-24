#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include <cstring>

#include "config.h"
#include "secrets.h"

// ======================================================
// BME280 settings
// ======================================================

const uint8_t GREENHOUSE_BME280_ADDRESS = 0x76;
const float SEALEVELPRESSURE_HPA = 1013.25;

// ======================================================
// Global objects
// ======================================================

Adafruit_BME280 bme;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool sensorReady = false;

// ======================================================
// Helper functions
// ======================================================

const char* toOnOff(bool isOn) {
    return isOn ? "ON" : "OFF";
}

const char* toMode(bool isAutoMode) {
    return isAutoMode ? "AUTO" : "MANUAL";
}

bool canPublishMqtt() {
    return WiFi.status() == WL_CONNECTED && mqttClient.connected();
}

// ======================================================
// Forward declarations
// ======================================================

void publishSensorData();
void publishActuatorStatus(const char* reason);

// ======================================================
// Fan state
// ======================================================

bool fanIsOn = false;
bool fanAutoMode = false;
float fanThreshold = DEFAULT_FAN_THRESHOLD_C;

// ======================================================
// Pump state
// ======================================================

bool pumpIsOn = false;
bool pumpAutoMode = true;
int soilMoistureThreshold = DEFAULT_SOIL_MOISTURE_THRESHOLD_PERCENT;

unsigned long pumpTurnedOnAt = 0;
unsigned long lastAutomaticPumpRun = 0;

// ======================================================
// Timers
// ======================================================

unsigned long lastSensorRead = 0;
unsigned long lastWifiReconnectAttempt = 0;
unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastDebugPrint = 0;

// ======================================================
// Publish actuator/device status
// This goes to MQTT_TOPIC_DEVICE_STATUS
// ======================================================

void publishActuatorStatus(const char* reason) {
    if (!canPublishMqtt()) {
        Serial.println("MQTT not connected. Skipping actuator status publish.");
        return;
    }

    StaticJsonDocument<700> doc;

    doc["deviceId"] = DEVICE_ID;
    doc["online"] = true;
    doc["status"] = "ONLINE";
    doc["reason"] = reason;

    doc["fanOn"] = fanIsOn;
    doc["fanState"] = toOnOff(fanIsOn);
    doc["fanAutoMode"] = fanAutoMode;
    doc["fanMode"] = toMode(fanAutoMode);
    doc["fanThreshold"] = fanThreshold;

    doc["pumpOn"] = pumpIsOn;
    doc["pumpState"] = toOnOff(pumpIsOn);
    doc["pumpAutoMode"] = pumpAutoMode;
    doc["pumpMode"] = toMode(pumpAutoMode);
    doc["soilMoistureThreshold"] = soilMoistureThreshold;

    char mqttPayload[700];
    serializeJson(doc, mqttPayload, sizeof(mqttPayload));

    bool success = mqttClient.publish(
        MQTT_TOPIC_DEVICE_STATUS,
        mqttPayload,
        true
    );

    Serial.print("Device status payload: ");
    Serial.println(mqttPayload);

    Serial.print("Device status publish: ");
    Serial.println(success ? "success" : "failed");
}

// ======================================================
// Fan relay control
// ======================================================

void setFan(bool turnOn) {
    fanIsOn = turnOn;

    if (FAN_RELAY_ACTIVE_LOW) {
        digitalWrite(FAN_RELAY_PIN, turnOn ? LOW : HIGH);
    } else {
        digitalWrite(FAN_RELAY_PIN, turnOn ? HIGH : LOW);
    }

    Serial.print("Fan: ");
    Serial.println(fanIsOn ? "ON" : "OFF");
}

void setupFan() {
    pinMode(FAN_RELAY_PIN, OUTPUT);
    setFan(false);
}

void updateFanAutomatic(float temperature) {
    if (!fanAutoMode) {
        return;
    }

    if (!fanIsOn && temperature >= fanThreshold) {
        Serial.println("Auto fan: temperature is high. Turning fan ON.");
        setFan(true);
        publishActuatorStatus("fan-auto-on");
    }

    if (fanIsOn && temperature <= fanThreshold - FAN_HYSTERESIS_C) {
        Serial.println("Auto fan: temperature is low enough. Turning fan OFF.");
        setFan(false);
        publishActuatorStatus("fan-auto-off");
    }
}

// ======================================================
// Pump relay control
// ======================================================

void setPump(bool turnOn) {
    pumpIsOn = turnOn;

    if (PUMP_RELAY_ACTIVE_LOW) {
        digitalWrite(WATER_PUMP_RELAY_PIN, turnOn ? LOW : HIGH);
    } else {
        digitalWrite(WATER_PUMP_RELAY_PIN, turnOn ? HIGH : LOW);
    }

    if (turnOn) {
        pumpTurnedOnAt = millis();
    }

    Serial.print("Pump: ");
    Serial.println(pumpIsOn ? "ON" : "OFF");
}

void setupPump() {
    pinMode(WATER_PUMP_RELAY_PIN, OUTPUT);
    setPump(false);
}

void updatePumpSafetyTimer() {
    if (!pumpIsOn) {
        return;
    }

    unsigned long now = millis();

    if (now - pumpTurnedOnAt >= PUMP_MAX_ON_TIME_MS) {
        Serial.println("Pump safety timer: turning pump OFF.");
        setPump(false);
        publishActuatorStatus("pump-safety-off");
    }
}

void updatePumpAutomatic(int soilMoisturePercent) {
    if (!pumpAutoMode) {
        return;
    }

    if (pumpIsOn) {
        return;
    }

    unsigned long now = millis();

    if (lastAutomaticPumpRun > 0 && now - lastAutomaticPumpRun < PUMP_COOLDOWN_MS) {
        return;
    }

    if (soilMoisturePercent < soilMoistureThreshold) {
        Serial.println("Auto pump: soil moisture is too low. Turning pump ON.");
        setPump(true);
        publishActuatorStatus("pump-auto-on");
        lastAutomaticPumpRun = now;
    }
}

// ======================================================
// Soil moisture sensor
// ======================================================

int readSoilMoistureRaw() {
    return analogRead(SOIL_MOISTURE_PIN);
}

int convertSoilMoistureToPercent(int rawValue) {
    long percentage = map(
        rawValue,
        SOIL_DRY_VALUE,
        SOIL_WET_VALUE,
        0,
        100
    );

    percentage = constrain(percentage, 0L, 100L);

    return (int)percentage;
}

void printSoilMoistureDebug() {
    int soilRaw = readSoilMoistureRaw();
    int soilPercent = convertSoilMoistureToPercent(soilRaw);

    Serial.print("Soil raw: ");
    Serial.print(soilRaw);

    Serial.print(" | Soil moisture: ");
    Serial.print(soilPercent);
    Serial.print("%");

    Serial.print(" | Pump state: ");
    Serial.print(toOnOff(pumpIsOn));

    Serial.print(" | Pump mode: ");
    Serial.print(toMode(pumpAutoMode));

    Serial.print(" | Fan state: ");
    Serial.print(toOnOff(fanIsOn));

    Serial.print(" | Fan mode: ");
    Serial.print(toMode(fanAutoMode));

    Serial.print(" | Pump threshold: ");
    Serial.print(soilMoistureThreshold);
    Serial.println("%");
}

// ======================================================
// MQTT command handlers
// ======================================================

void handleFanCommand(String message) {
    message.trim();

    if (message == "on") {
        fanAutoMode = false;
        setFan(true);
        Serial.println("Fan mode: MANUAL");
        return;
    }

    if (message == "off") {
        fanAutoMode = false;
        setFan(false);
        Serial.println("Fan mode: MANUAL");
        return;
    }

    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("Fan JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* mode = doc["mode"] | "";
    const char* state = doc["state"] | "";

    if (strcmp(mode, "auto") == 0) {
        fanAutoMode = true;
        fanThreshold = doc["threshold"] | fanThreshold;

        Serial.print("Fan mode: AUTO. Threshold: ");
        Serial.println(fanThreshold);

        if (sensorReady) {
            float temperature = bme.readTemperature();
            updateFanAutomatic(temperature);
        }

        return;
    }

    if (strcmp(mode, "manual") == 0) {
        fanAutoMode = false;

        if (strcmp(state, "on") == 0) {
            setFan(true);
        } else {
            setFan(false);
        }

        Serial.println("Fan mode: MANUAL");
        return;
    }

    if (strcmp(state, "on") == 0) {
        fanAutoMode = false;
        setFan(true);
        Serial.println("Fan mode: MANUAL");
        return;
    }

    if (strcmp(state, "off") == 0) {
        fanAutoMode = false;
        setFan(false);
        Serial.println("Fan mode: MANUAL");
        return;
    }

    Serial.println("Unknown fan command.");
}

void handlePumpCommand(String message) {
    message.trim();

    if (message == "on") {
        pumpAutoMode = false;
        setPump(true);
        Serial.println("Pump mode: MANUAL");
        return;
    }

    if (message == "off") {
        pumpAutoMode = false;
        setPump(false);
        Serial.println("Pump mode: MANUAL");
        return;
    }

    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("Pump JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* mode = doc["mode"] | "";
    const char* state = doc["state"] | "";

    if (strcmp(mode, "auto") == 0) {
        pumpAutoMode = true;

        soilMoistureThreshold = doc["threshold"] | soilMoistureThreshold;
        soilMoistureThreshold = constrain(soilMoistureThreshold, 0, 100);

        Serial.print("Pump mode: AUTO. Soil threshold: ");
        Serial.print(soilMoistureThreshold);
        Serial.println("%");

        int soilRaw = readSoilMoistureRaw();
        int soilPercent = convertSoilMoistureToPercent(soilRaw);
        updatePumpAutomatic(soilPercent);

        return;
    }

    if (strcmp(mode, "manual") == 0) {
        pumpAutoMode = false;

        if (strcmp(state, "on") == 0) {
            setPump(true);
        } else {
            setPump(false);
        }

        Serial.println("Pump mode: MANUAL");
        return;
    }

    if (strcmp(state, "on") == 0) {
        pumpAutoMode = false;
        setPump(true);
        Serial.println("Pump mode: MANUAL");
        return;
    }

    if (strcmp(state, "off") == 0) {
        pumpAutoMode = false;
        setPump(false);
        Serial.println("Pump mode: MANUAL");
        return;
    }

    Serial.println("Unknown pump command.");
}

// ======================================================
// MQTT callback
// ======================================================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";

    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    message.trim();

    Serial.println();
    Serial.print("MQTT command topic: ");
    Serial.println(topic);

    Serial.print("MQTT command payload: ");
    Serial.println(message);

    if (String(topic) == MQTT_TOPIC_FAN_COMMAND) {
        handleFanCommand(message);
        publishActuatorStatus("fan-command");
        publishSensorData();
        return;
    }

    if (String(topic) == MQTT_TOPIC_PUMP_COMMAND) {
        handlePumpCommand(message);
        publishActuatorStatus("pump-command");
        publishSensorData();
        return;
    }

    Serial.println("Unknown MQTT topic.");
}

// ======================================================
// WiFi events
// ======================================================

void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi connected to access point");
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Got IP address: ");
            Serial.println(WiFi.localIP());
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi disconnected");
            break;

        default:
            break;
    }
}

// ======================================================
// WiFi connection
// ======================================================

bool connectWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connection successful!");
        return true;
    }

    Serial.println("WiFi connection failed!");
    return false;
}

void reconnectWiFiIfNeeded() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();

    if (now - lastWifiReconnectAttempt >= 5000) {
        lastWifiReconnectAttempt = now;
        Serial.println("Reconnecting to WiFi...");
        WiFi.reconnect();
    }
}

// ======================================================
// MQTT connection
// ======================================================

bool connectMQTT() {
    if (mqttClient.connected()) {
        return true;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot connect MQTT because WiFi is disconnected.");
        return false;
    }

    Serial.print("Connecting to MQTT broker: ");
    Serial.println(MQTT_BROKER);

    String clientId = "greenhouse-esp32-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    String offlinePayload = "{\"deviceId\":\"";
    offlinePayload += DEVICE_ID;
    offlinePayload += "\",\"online\":false,\"status\":\"OFFLINE\"}";

    bool connected = mqttClient.connect(
        clientId.c_str(),
        MQTT_TOKEN,
        "",
        MQTT_TOPIC_DEVICE_STATUS,
        1,
        true,
        offlinePayload.c_str()
    );

    if (connected) {
        Serial.println("MQTT connected!");

        mqttClient.subscribe(MQTT_TOPIC_FAN_COMMAND);
        mqttClient.subscribe(MQTT_TOPIC_PUMP_COMMAND);

        Serial.print("Subscribed to fan command topic: ");
        Serial.println(MQTT_TOPIC_FAN_COMMAND);

        Serial.print("Subscribed to pump command topic: ");
        Serial.println(MQTT_TOPIC_PUMP_COMMAND);

        publishActuatorStatus("mqtt-connected");

        return true;
    }

    Serial.print("MQTT connection failed. State: ");
    Serial.println(mqttClient.state());

    return false;
}

void reconnectMQTTIfNeeded() {
    if (mqttClient.connected()) {
        return;
    }

    unsigned long now = millis();

    if (now - lastMqttReconnectAttempt >= 5000) {
        lastMqttReconnectAttempt = now;
        connectMQTT();
    }
}

// ======================================================
// BME280 sensor
// ======================================================

bool initSensor() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (!bme.begin(GREENHOUSE_BME280_ADDRESS)) {
        Serial.println("ERROR: BME280/BMP280 sensor not found!");
        Serial.println("Check address 0x76/0x77 and wiring.");
        return false;
    }

    Serial.println("BME280/BMP280 sensor initialized successfully!");
    return true;
}

// ======================================================
// Publish sensor data
// This goes to MQTT_TOPIC_SENSOR_DATA
// ======================================================

void publishSensorData() {
    if (!sensorReady) {
        Serial.println("Sensor not connected. Skipping MQTT publish.");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected. Skipping MQTT publish.");
        return;
    }

    if (!mqttClient.connected()) {
        Serial.println("MQTT disconnected. Skipping MQTT publish.");
        return;
    }

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0;
    float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

    int soilRaw = readSoilMoistureRaw();
    int soilPercent = convertSoilMoistureToPercent(soilRaw);

    updateFanAutomatic(temperature);
    updatePumpAutomatic(soilPercent);

    StaticJsonDocument<1000> doc;

    doc["deviceId"] = DEVICE_ID;
    doc["online"] = true;

    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["pressure"] = pressure;
    doc["altitude"] = altitude;

    doc["soilMoistureRaw"] = soilRaw;
    doc["soilMoisture"] = soilPercent;

    doc["fanOn"] = fanIsOn;
    doc["fanState"] = toOnOff(fanIsOn);
    doc["fanAutoMode"] = fanAutoMode;
    doc["fanMode"] = toMode(fanAutoMode);
    doc["fanThreshold"] = fanThreshold;

    doc["pumpOn"] = pumpIsOn;
    doc["pumpState"] = toOnOff(pumpIsOn);
    doc["pumpAutoMode"] = pumpAutoMode;
    doc["pumpMode"] = toMode(pumpAutoMode);
    doc["soilMoistureThreshold"] = soilMoistureThreshold;

    char mqttPayload[1000];
    serializeJson(doc, mqttPayload, sizeof(mqttPayload));

    bool success = mqttClient.publish(MQTT_TOPIC_SENSOR_DATA, mqttPayload);

    Serial.println();
    Serial.println("Sensor readings");
    Serial.println("----------------------");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");

    Serial.print("Altitude: ");
    Serial.print(altitude);
    Serial.println(" m");

    Serial.print("Soil raw: ");
    Serial.println(soilRaw);

    Serial.print("Soil moisture: ");
    Serial.print(soilPercent);
    Serial.println(" %");

    Serial.print("Fan: ");
    Serial.println(fanIsOn ? "ON" : "OFF");

    Serial.print("Fan mode: ");
    Serial.println(fanAutoMode ? "AUTO" : "MANUAL");

    Serial.print("Fan threshold: ");
    Serial.print(fanThreshold);
    Serial.println(" °C");

    Serial.print("Pump: ");
    Serial.println(pumpIsOn ? "ON" : "OFF");

    Serial.print("Pump mode: ");
    Serial.println(pumpAutoMode ? "AUTO" : "MANUAL");

    Serial.print("Soil threshold: ");
    Serial.print(soilMoistureThreshold);
    Serial.println(" %");

    Serial.print("MQTT payload: ");
    Serial.println(mqttPayload);

    Serial.print("Publish status: ");
    Serial.println(success ? "success" : "failed");
}

// ======================================================
// Setup
// ======================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    setupFan();
    setupPump();

    pinMode(SOIL_MOISTURE_PIN, INPUT);

    analogReadResolution(12);

    Serial.println();
    Serial.println("GreenHouse IoT - BME280 + MQTT + Fan + Pump + Soil Moisture");
    Serial.println("============================================================");

    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);

    Serial.print("Soil moisture pin: GPIO ");
    Serial.println(SOIL_MOISTURE_PIN);

    Serial.print("Fan relay pin: GPIO ");
    Serial.println(FAN_RELAY_PIN);

    Serial.print("Pump relay pin: GPIO ");
    Serial.println(WATER_PUMP_RELAY_PIN);

    Serial.print("Default soil threshold: ");
    Serial.print(soilMoistureThreshold);
    Serial.println("%");

    WiFi.onEvent(onWiFiEvent);
    connectWiFi();

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setBufferSize(1000);
    mqttClient.setCallback(mqttCallback);

    connectMQTT();

    sensorReady = initSensor();

    publishActuatorStatus("setup-finished");
}

// ======================================================
// Loop
// ======================================================

void loop() {
    reconnectWiFiIfNeeded();
    reconnectMQTTIfNeeded();

    mqttClient.loop();

    updatePumpSafetyTimer();

    unsigned long now = millis();

    if (now - lastDebugPrint >= DEBUG_PRINT_INTERVAL_MS) {
        lastDebugPrint = now;
        printSoilMoistureDebug();
    }

    if (now - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
        lastSensorRead = now;
        publishSensorData();
    }
}