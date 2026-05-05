#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "config.h"

#define BME280_ADDRESS 0x76
#define SEALEVELPRESSURE_HPA 1013.25

Adafruit_BME280 bme;

unsigned long lastSensorRead = 0;
unsigned long lastRelayToggle = 0;

bool relayState = false;

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
}

void toggleRelay() {
    relayState = !relayState;

    digitalWrite(WATER_PUMP_RELAY_PIN, relayState ? HIGH : LOW);

    Serial.print("Relay state: ");
    Serial.println(relayState ? "ON" : "OFF");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("BME280 + Relay Test");
    Serial.println("===================");

    Serial.print("SDA: GPIO ");
    Serial.println(I2C_SDA_PIN);

    Serial.print("SCL: GPIO ");
    Serial.println(I2C_SCL_PIN);

    Serial.print("I2C address: 0x");
    Serial.println(BME280_ADDRESS, HEX);

    Serial.print("Relay pin: GPIO ");
    Serial.println(WATER_PUMP_RELAY_PIN);

    pinMode(WATER_PUMP_RELAY_PIN, OUTPUT);
    digitalWrite(WATER_PUMP_RELAY_PIN, LOW);

    if (!initSensor()) {
        while (true) {
            delay(1000);
        }
    }

    Serial.println("Sensor initialized successfully!");
    Serial.println("Relay initialized successfully!");
}

void loop() {
    unsigned long now = millis();

    if (now - lastSensorRead >= 2000) {
        lastSensorRead = now;
        readSensor();
    }

    if (now - lastRelayToggle >= 2000) {
        lastRelayToggle = now;
        toggleRelay();
    }
}