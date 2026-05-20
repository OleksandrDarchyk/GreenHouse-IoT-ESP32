#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include "config.h"
#include "secrets.h"

#define BME280_ADDRESS 0x76
#define SEALEVELPRESSURE_HPA 1013.25

Adafruit_BME280 bme;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool sensorReady = false;
bool fanIsOn = false;
bool fanAutoMode = false;

float fanThreshold = DEFAULT_FAN_THRESHOLD_C;

unsigned long lastSensorRead = 0;
unsigned long lastWifiReconnectAttempt = 0;
unsigned long lastMqttReconnectAttempt = 0;

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
    }

    if (fanIsOn && temperature <= fanThreshold - FAN_HYSTERESIS_C) {
        Serial.println("Auto fan: temperature is low enough. Turning fan OFF.");
        setFan(false);
    }
}

// ======================================================
// MQTT command handler
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

    if (String(topic) != MQTT_TOPIC_FAN_COMMAND) {
        return;
    }

    // Manual simple commands: on / off
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

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* mode = doc["mode"] | "";

    if (strcmp(mode, "auto") == 0) {
        fanAutoMode = true;
        fanThreshold = doc["threshold"] | fanThreshold;

        Serial.print("Fan mode: AUTO. Threshold: ");
        Serial.println(fanThreshold);

        if (sensorReady) {
            updateFanAutomatic(bme.readTemperature());
        }

        return;
    }

    if (strcmp(mode, "manual") == 0) {
        fanAutoMode = false;

        const char* state = doc["state"] | "off";

        if (strcmp(state, "on") == 0) {
            setFan(true);
        } else {
            setFan(false);
        }

        Serial.println("Fan mode: MANUAL");
        return;
    }

    Serial.println("Unknown fan command.");
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

    bool connected = mqttClient.connect(
        clientId.c_str(),
        MQTT_TOKEN,
        ""
    );

    if (connected) {
        Serial.println("MQTT connected!");

        mqttClient.subscribe(MQTT_TOPIC_FAN_COMMAND);

        Serial.print("Subscribed to fan command topic: ");
        Serial.println(MQTT_TOPIC_FAN_COMMAND);

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

    if (!bme.begin(BME280_ADDRESS)) {
        Serial.println("ERROR: BME280/BMP280 sensor not found!");
        Serial.println("Check address 0x76/0x77 and wiring.");
        return false;
    }

    Serial.println("Sensor initialized successfully!");
    return true;
}

// ======================================================
// Publish sensor data
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

    updateFanAutomatic(temperature);

    char mqttPayload[420];

    snprintf(
        mqttPayload,
        sizeof(mqttPayload),
        "{\"deviceId\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f,\"altitude\":%.2f,\"fanOn\":%s,\"fanAutoMode\":%s,\"fanThreshold\":%.2f}",
        DEVICE_ID,
        temperature,
        humidity,
        pressure,
        altitude,
        fanIsOn ? "true" : "false",
        fanAutoMode ? "true" : "false",
        fanThreshold
    );

    bool success = mqttClient.publish(MQTT_TOPIC_SENSOR_DATA, mqttPayload);

    Serial.println();
    Serial.println("BME280 readings");
    Serial.println("----------------------");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity:    ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure:    ");
    Serial.print(pressure);
    Serial.println(" hPa");

    Serial.print("Altitude:    ");
    Serial.print(altitude);
    Serial.println(" m");

    Serial.print("Fan:         ");
    Serial.println(fanIsOn ? "ON" : "OFF");

    Serial.print("Fan mode:    ");
    Serial.println(fanAutoMode ? "AUTO" : "MANUAL");

    Serial.print("Threshold:   ");
    Serial.print(fanThreshold);
    Serial.println(" °C");

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

    Serial.println();
    Serial.println("GreenHouse IoT - BME280 + MQTT + Fan Auto Control");
    Serial.println("=================================================");

    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);

    Serial.print("Fan relay pin: GPIO ");
    Serial.println(FAN_RELAY_PIN);

    WiFi.onEvent(onWiFiEvent);
    connectWiFi();

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    connectMQTT();

    sensorReady = initSensor();
}

// ======================================================
// Loop
// ======================================================

void loop() {
    reconnectWiFiIfNeeded();
    reconnectMQTTIfNeeded();

    mqttClient.loop();

    unsigned long now = millis();

    if (now - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
        lastSensorRead = now;
        publishSensorData();
    }
}