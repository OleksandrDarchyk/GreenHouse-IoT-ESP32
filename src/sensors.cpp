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
    analogReadResolution(ADC_RESOLUTION_BITS);
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

// ======================================================
// LDR light sensor
// ======================================================

void setupLightSensor() {
    pinMode(LDR_PIN, INPUT);
    analogReadResolution(ADC_RESOLUTION_BITS);
}

int readLightRaw() {
    return analogRead(LDR_PIN);
}

int convertLightToPercent(int rawValue) {
    // ESP32 ADC range: 0-4095.
    // more light  give a higher raw value.
    long percentage = map(rawValue, 0, ADC_MAX_VALUE, 0, 100);

    percentage = constrain(percentage, 0L, 100L);

    return (int)percentage;
}

// ======================================================
// Debug print
// ======================================================

void printSoilMoistureDebug() {
    int soilRaw = readSoilMoistureRaw();
    int soilPercent = convertSoilMoistureToPercent(soilRaw);

    int lightRaw = readLightRaw();
    int lightPercent = convertLightToPercent(lightRaw);

    Serial.print("Soil raw: ");
    Serial.print(soilRaw);

    Serial.print(" | Soil moisture: ");
    Serial.print(soilPercent);
    Serial.print("%");

    Serial.print(" | Light raw: ");
    Serial.print(lightRaw);

    Serial.print(" | Light: ");
    Serial.print(lightPercent);
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
    SensorData data = {};

    if (sensorReady) {
        data.temperature = bme.readTemperature();
        data.humidity = bme.readHumidity();
        data.pressure = bme.readPressure() / 100.0;
        data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    } else {
        data.temperature = NAN;
        data.humidity = NAN;
        data.pressure = NAN;
        data.altitude = NAN;

        Serial.println("BME280 is not ready. BME280 values are invalid.");
    }

    data.soilRaw = readSoilMoistureRaw();
    data.soilMoisturePercent = convertSoilMoistureToPercent(data.soilRaw);

    data.lightRaw = readLightRaw();
    data.lightPercent = convertLightToPercent(data.lightRaw);

    return data;
}