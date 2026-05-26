#include <Arduino.h>

#include "config.h"
#include "sensors.h"
#include "actuators.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

// ======================================================
// Timers
// ======================================================

unsigned long lastSensorRead = 0;
unsigned long lastDebugPrint = 0;

// ======================================================
// Setup
// ======================================================

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(STARTUP_DELAY_MS);

    setupActuators();
    setupSoilMoistureSensor();
    setupLightSensor();

    Serial.println();
    Serial.println("GreenHouse IoT - BME280 + MQTT + Fan + Pump + Soil Moisture + Light");
    Serial.println("====================================================================");

    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);

    Serial.print("Soil moisture pin: GPIO ");
    Serial.println(SOIL_MOISTURE_PIN);

    Serial.print("Light sensor pin: GPIO ");
    Serial.println(LDR_PIN);

    Serial.print("Fan relay pin: GPIO ");
    Serial.println(FAN_RELAY_PIN);

    Serial.print("Pump relay pin: GPIO ");
    Serial.println(WATER_PUMP_RELAY_PIN);

    Serial.print("Default soil threshold: ");
    Serial.print(getSoilMoistureThreshold());
    Serial.println("%");

    setupWiFiEvents();
    connectWiFi();

    setupMqttClient();
    connectMQTT();

    initSensor();

    publishActuatorStatus("setup-finished");
}

// ======================================================
// Loop
// ======================================================

void loop() {
    reconnectWiFiIfNeeded();
    reconnectMQTTIfNeeded();

    mqttLoop();

    updatePumpSafetyTimer();

    unsigned long now = millis();

    if (now - lastDebugPrint >= DEBUG_PRINT_INTERVAL_MS) {
        lastDebugPrint = now;
        printSoilMoistureDebug();
    }

    if (now - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
        lastSensorRead = now;
        publishSensorData();
    }
}