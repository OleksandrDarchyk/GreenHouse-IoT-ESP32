#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "config.h"
#include "secrets.h"

#define BME280_ADDRESS 0x76
#define SEALEVELPRESSURE_HPA 1013.25

Adafruit_BME280 bme;

bool sensorReady = false;
unsigned long lastSensorRead = 0;
unsigned long lastReconnectAttempt = 0;

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
        Serial.println("Connection successful!");
        return true;
    }

    Serial.println("WiFi connection failed!");
    return false;
}

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

void readSensor() {
    if (!sensorReady) {
        Serial.println("Sensor not connected. Skipping BME280 readings.");
        return;
    }

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0;
    float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

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

    Serial.print("WiFi status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");
}

void reconnectWiFiIfNeeded() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();

    if (now - lastReconnectAttempt >= 5000) {
        lastReconnectAttempt = now;

        Serial.println("Reconnecting to WiFi...");
        WiFi.reconnect();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("GreenHouse IoT - BME280 + WiFi Test");
    Serial.println("===================================");

    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);

    Serial.print("SDA: GPIO ");
    Serial.println(I2C_SDA_PIN);

    Serial.print("SCL: GPIO ");
    Serial.println(I2C_SCL_PIN);

    Serial.print("I2C address: 0x");
    Serial.println(BME280_ADDRESS, HEX);

    WiFi.onEvent(onWiFiEvent);
    connectWiFi();

    sensorReady = initSensor();
}

void loop() {
    reconnectWiFiIfNeeded();

    unsigned long now = millis();

    if (now - lastSensorRead >= 5000) {
        lastSensorRead = now;
        readSensor();
    }
}