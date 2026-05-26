#include "actuators.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstring>

#include "config.h"
#include "sensors.h"
#include "mqtt_manager.h"

// ======================================================
// Helper functions
// ======================================================

const char* toOnOff(bool isOn) {
    return isOn ? "ON" : "OFF";
}

const char* toMode(bool isAutoMode) {
    return isAutoMode ? "AUTO" : "MANUAL";
}

// ======================================================
// Fan state
// ======================================================

bool fanIsOn = false;
bool fanAutoMode = false;
float fanThreshold = DEFAULT_FAN_THRESHOLD_C;

// ======================================================
// Pump state
// ======================================================

bool pumpIsOn = false;
bool pumpAutoMode = true;
int soilMoistureThreshold = DEFAULT_SOIL_MOISTURE_THRESHOLD_PERCENT;

unsigned long pumpTurnedOnAt = 0;
unsigned long lastAutomaticPumpRun = 0;

// ======================================================
// Getters
// ======================================================

bool isFanOn() {
    return fanIsOn;
}

bool isPumpOn() {
    return pumpIsOn;
}

bool isFanAutoMode() {
    return fanAutoMode;
}

bool isPumpAutoMode() {
    return pumpAutoMode;
}

float getFanThreshold() {
    return fanThreshold;
}

int getSoilMoistureThreshold() {
    return soilMoistureThreshold;
}

// ======================================================
// Fan relay control
// ======================================================

void setFan(bool turnOn) {
    fanIsOn = turnOn;

    if (FAN_RELAY_ACTIVE_LOW) {
        digitalWrite(FAN_RELAY_PIN, turnOn ? LOW : HIGH);
    } else {
        digitalWrite(FAN_RELAY_PIN, turnOn ? HIGH : LOW);
    }

    Serial.print("Fan: ");
    Serial.println(fanIsOn ? "ON" : "OFF");
}

void setupFan() {
    pinMode(FAN_RELAY_PIN, OUTPUT);
    setFan(false);
}

void updateFanAutomatic(float temperature) {
    if (!fanAutoMode) {
        return;
    }

    if (!fanIsOn && temperature >= fanThreshold) {
        Serial.println("Auto fan: temperature is high. Turning fan ON.");
        setFan(true);
        publishActuatorStatus("fan-auto-on");
    }

    if (fanIsOn && temperature <= fanThreshold - FAN_HYSTERESIS_C) {
        Serial.println("Auto fan: temperature is low enough. Turning fan OFF.");
        setFan(false);
        publishActuatorStatus("fan-auto-off");
    }
}

// ======================================================
// Pump relay control
// ======================================================

void setPump(bool turnOn) {
    pumpIsOn = turnOn;

    if (PUMP_RELAY_ACTIVE_LOW) {
        digitalWrite(WATER_PUMP_RELAY_PIN, turnOn ? LOW : HIGH);
    } else {
        digitalWrite(WATER_PUMP_RELAY_PIN, turnOn ? HIGH : LOW);
    }

    if (turnOn) {
        pumpTurnedOnAt = millis();
    }

    Serial.print("Pump: ");
    Serial.println(pumpIsOn ? "ON" : "OFF");
}

void setupPump() {
    pinMode(WATER_PUMP_RELAY_PIN, OUTPUT);
    setPump(false);
}

void setupActuators() {
    setupFan();
    setupPump();
}

void updatePumpSafetyTimer() {
    if (!pumpIsOn) {
        return;
    }

    unsigned long now = millis();

    if (now - pumpTurnedOnAt >= PUMP_MAX_ON_TIME_MS) {
        Serial.println("Pump safety timer: turning pump OFF.");
        setPump(false);
        publishActuatorStatus("pump-safety-off");
    }
}

void updatePumpAutomatic(int soilMoisturePercent) {
    if (!pumpAutoMode) {
        return;
    }

    if (pumpIsOn) {
        return;
    }

    unsigned long now = millis();

    if (lastAutomaticPumpRun > 0 && now - lastAutomaticPumpRun < PUMP_COOLDOWN_MS) {
        return;
    }

    if (soilMoisturePercent < soilMoistureThreshold) {
        Serial.println("Auto pump: soil moisture is too low. Turning pump ON.");
        setPump(true);
        publishActuatorStatus("pump-auto-on");
        lastAutomaticPumpRun = now;
    }
}

// ======================================================
// MQTT command handlers
// ======================================================

void handleFanCommand(String message) {
    message.trim();

    if (message == "on") {
        fanAutoMode = false;
        setFan(true);
        Serial.println("Fan mode: MANUAL");
        return;
    }

    if (message == "off") {
        fanAutoMode = false;
        setFan(false);
        Serial.println("Fan mode: MANUAL");
        return;
    }

    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("Fan JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* mode = doc["mode"] | "";
    const char* state = doc["state"] | "";

    if (strcmp(mode, "auto") == 0) {
        fanAutoMode = true;
        fanThreshold = doc["threshold"] | fanThreshold;

        Serial.print("Fan mode: AUTO. Threshold: ");
        Serial.println(fanThreshold);

        if (isSensorReady()) {
            float temperature = readTemperature();
            updateFanAutomatic(temperature);
        }

        return;
    }

    if (strcmp(mode, "manual") == 0) {
        fanAutoMode = false;

        if (strcmp(state, "on") == 0) {
            setFan(true);
        } else {
            setFan(false);
        }

        Serial.println("Fan mode: MANUAL");
        return;
    }

    if (strcmp(state, "on") == 0) {
        fanAutoMode = false;
        setFan(true);
        Serial.println("Fan mode: MANUAL");
        return;
    }

    if (strcmp(state, "off") == 0) {
        fanAutoMode = false;
        setFan(false);
        Serial.println("Fan mode: MANUAL");
        return;
    }

    Serial.println("Unknown fan command.");
}

void handlePumpCommand(String message) {
    message.trim();

    if (message == "on") {
        pumpAutoMode = false;
        setPump(true);
        Serial.println("Pump mode: MANUAL");
        return;
    }

    if (message == "off") {
        pumpAutoMode = false;
        setPump(false);
        Serial.println("Pump mode: MANUAL");
        return;
    }

    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("Pump JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* mode = doc["mode"] | "";
    const char* state = doc["state"] | "";

    if (strcmp(mode, "auto") == 0) {
        pumpAutoMode = true;

        soilMoistureThreshold = doc["threshold"] | soilMoistureThreshold;
        soilMoistureThreshold = constrain(soilMoistureThreshold, 0, 100);

        Serial.print("Pump mode: AUTO. Soil threshold: ");
        Serial.print(soilMoistureThreshold);
        Serial.println("%");

        int soilRaw = readSoilMoistureRaw();
        int soilPercent = convertSoilMoistureToPercent(soilRaw);
        updatePumpAutomatic(soilPercent);

        return;
    }

    if (strcmp(mode, "manual") == 0) {
        pumpAutoMode = false;

        if (strcmp(state, "on") == 0) {
            setPump(true);
        } else {
            setPump(false);
        }

        Serial.println("Pump mode: MANUAL");
        return;
    }

    if (strcmp(state, "on") == 0) {
        pumpAutoMode = false;
        setPump(true);
        Serial.println("Pump mode: MANUAL");
        return;
    }

    if (strcmp(state, "off") == 0) {
        pumpAutoMode = false;
        setPump(false);
        Serial.println("Pump mode: MANUAL");
        return;
    }

    Serial.println("Unknown pump command.");
}