#ifndef SENSORS_H
#define SENSORS_H

struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    float altitude;
    int soilRaw;
    int soilMoisturePercent;
};

void setupSoilMoistureSensor();
bool initSensor();

bool isSensorReady();

float readTemperature();
SensorData readSensorData();

int readSoilMoistureRaw();
int convertSoilMoistureToPercent(int rawValue);

void printSoilMoistureDebug();

#endif