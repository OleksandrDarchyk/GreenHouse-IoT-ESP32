#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>

void setupFan();
void setupPump();
void setupActuators();

void setFan(bool turnOn);
void setPump(bool turnOn);

void updateFanAutomatic(float temperature);
void updatePumpAutomatic(int soilMoisturePercent);
void updatePumpSafetyTimer();

void handleFanCommand(String message);
void handlePumpCommand(String message);

bool isFanOn();
bool isPumpOn();

bool isFanAutoMode();
bool isPumpAutoMode();

float getFanThreshold();
int getSoilMoistureThreshold();

const char* toOnOff(bool isOn);
const char* toMode(bool isAutoMode);

#endif