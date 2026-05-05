#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "config.h"

#define BME280_ADDRESS 0x76
#define SEALEVELPRESSURE_HPA 1013.25

Adafruit_BME280 bme;

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

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("BME280 Sensor Example");
    Serial.println("=====================");

    Serial.print("SDA: GPIO ");
    Serial.println(I2C_SDA_PIN);

    Serial.print("SCL: GPIO ");
    Serial.println(I2C_SCL_PIN);

    Serial.print("I2C address: 0x");
    Serial.println(BME280_ADDRESS, HEX);

    if (!initSensor()) {
        while (true) {
            delay(1000);
        }
    }

    Serial.println("Sensor initialized successfully!");
}

void loop() {
    readSensor();
    delay(2000);
}