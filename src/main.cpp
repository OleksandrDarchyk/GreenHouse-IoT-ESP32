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

void connectWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi connection failed!");
    }
}

bool initSensor() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (!bme.begin(BME280_ADDRESS)) {
        Serial.println("ERROR: BME280/BMP280 sensor not found!");
        Serial.println("Check address 0x76/0x77 and wiring.");
        return false;
    }

    return true;
}

void readSensor() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0;
    float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

    Serial.println();
    Serial.println("BME280/BMP280 readings");
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

    connectWiFi();

    if (!initSensor()) {
        while (true) {
            delay(1000);
        }
    }

    Serial.println("Sensor initialized successfully!");
}

void loop() {
    readSensor();
    delay(5000);
}