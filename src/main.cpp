#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "config.h"
#include "secrets.h"

#define BME280_ADDRESS 0x76
#define SEALEVELPRESSURE_HPA 1013.25

Adafruit_BME280 bme;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool sensorReady = false;

unsigned long lastSensorRead = 0;
unsigned long lastWifiReconnectAttempt = 0;
unsigned long lastMqttReconnectAttempt = 0;

// ==========================
// WiFi events
// ==========================

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

// ==========================
// WiFi connection
// ==========================

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

// ==========================
// MQTT connection
// ==========================

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

// ==========================
// BME280 sensor
// ==========================

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

// ==========================
// Read + publish sensor data
// ==========================

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

    char payload[256];

    snprintf(
        payload,
        sizeof(payload),
        "{\"deviceId\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f,\"altitude\":%.2f}",
        DEVICE_ID,
        temperature,
        humidity,
        pressure,
        altitude
    );

    bool success = mqttClient.publish(MQTT_TOPIC_SENSOR_DATA, payload);

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

    Serial.print("MQTT topic:  ");
    Serial.println(MQTT_TOPIC_SENSOR_DATA);

    Serial.print("MQTT payload: ");
    Serial.println(payload);

    Serial.print("Publish status: ");
    Serial.println(success ? "success" : "failed");
}

// ==========================
// Setup
// ==========================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("GreenHouse IoT - BME280 + WiFi + MQTT");
    Serial.println("=====================================");

    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);

    Serial.print("SDA: GPIO ");
    Serial.println(I2C_SDA_PIN);

    Serial.print("SCL: GPIO ");
    Serial.println(I2C_SCL_PIN);

    Serial.print("I2C address: 0x");
    Serial.println(BME280_ADDRESS, HEX);

    Serial.print("MQTT broker: ");
    Serial.println(MQTT_BROKER);

    Serial.print("MQTT topic: ");
    Serial.println(MQTT_TOPIC_SENSOR_DATA);

    WiFi.onEvent(onWiFiEvent);
    connectWiFi();

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    connectMQTT();

    sensorReady = initSensor();
}

// ==========================
// Loop
// ==========================

void loop() {
    reconnectWiFiIfNeeded();
    reconnectMQTTIfNeeded();

    mqttClient.loop();

    unsigned long now = millis();

    if (now - lastSensorRead >= 5000) {
        lastSensorRead = now;
        publishSensorData();
    }
}