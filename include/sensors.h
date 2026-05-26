#ifndef SENSORS_H
#define SENSORS_H

struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    float altitude;

    int soilRaw;
    int soilMoisturePercent;

    int lightRaw;
    int lightPercent;
};

void setupSoilMoistureSensor();
void setupLightSensor();

bool initSensor();
bool isSensorReady();

float readTemperature();
SensorData readSensorData();

int readSoilMoistureRaw();
int convertSoilMoistureToPercent(int rawValue);

int readLightRaw();
int convertLightToPercent(int rawValue);

void printSoilMoistureDebug();

#endif