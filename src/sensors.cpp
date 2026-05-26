#include "sensors.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "config.h"
#include "actuators.h"

// ======================================================
// BME280 settings
// ======================================================

const uint8_t GREENHOUSE_BME280_ADDRESS = 0x76;
const float SEALEVELPRESSURE_HPA = 1013.25;

// ======================================================
// Global sensor objects/state
// ======================================================

Adafruit_BME280 bme;
bool sensorReady = false;

// ======================================================
// Soil moisture sensor
// ======================================================

void setupSoilMoistureSensor() {
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    analogReadResolution(12);
}

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
    Serial.print(toOnOff(isPumpOn()));

    Serial.print(" | Pump mode: ");
    Serial.print(toMode(isPumpAutoMode()));

    Serial.print(" | Fan state: ");
    Serial.print(toOnOff(isFanOn()));

    Serial.print(" | Fan mode: ");
    Serial.print(toMode(isFanAutoMode()));

    Serial.print(" | Pump threshold: ");
    Serial.print(getSoilMoistureThreshold());
    Serial.println("%");
}

// ======================================================
// BME280 sensor
// ======================================================

bool initSensor() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (!bme.begin(GREENHOUSE_BME280_ADDRESS)) {
        Serial.println("ERROR: BME280/BMP280 sensor not found!");
        Serial.println("Check address 0x76/0x77 and wiring.");
        sensorReady = false;
        return false;
    }

    Serial.println("BME280/BMP280 sensor initialized successfully!");
    sensorReady = true;
    return true;
}

bool isSensorReady() {
    return sensorReady;
}

float readTemperature() {
    if (!sensorReady) {
        return 0.0;
    }

    return bme.readTemperature();
}

SensorData readSensorData() {
    SensorData data;

    data.temperature = bme.readTemperature();
    data.humidity = bme.readHumidity();
    data.pressure = bme.readPressure() / 100.0;
    data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

    data.soilRaw = readSoilMoistureRaw();
    data.soilMoisturePercent = convertSoilMoistureToPercent(data.soilRaw);

    return data;
}