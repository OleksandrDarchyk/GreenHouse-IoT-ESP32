#include <Arduino.h>
#include "config.h"

unsigned long lastDebugPrint = 0;

void printStartupInfo() {
    Serial.println();
    Serial.println("=================================");
    Serial.println("GreenHouse IoT Firmware");
    Serial.println("Board: FireBeetle 2 ESP32-E");
    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);
    Serial.println("=================================");

    Serial.println("Configured pins:");
    Serial.print("Onboard LED: GPIO ");
    Serial.println(ONBOARD_LED_PIN);

    Serial.print("I2C SDA: GPIO ");
    Serial.println(I2C_SDA_PIN);

    Serial.print("I2C SCL: GPIO ");
    Serial.println(I2C_SCL_PIN);

    Serial.print("Soil moisture sensor: GPIO ");
    Serial.println(SOIL_MOISTURE_PIN);

    Serial.print("LDR sensor: GPIO ");
    Serial.println(LDR_PIN);

    Serial.print("MQ-135 sensor: GPIO ");
    Serial.println(MQ135_PIN);

    Serial.print("Water pump relay: GPIO ");
    Serial.println(WATER_PUMP_RELAY_PIN);

    Serial.print("Fan relay: GPIO ");
    Serial.println(FAN_RELAY_PIN);

    Serial.println("=================================");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(ONBOARD_LED_PIN, OUTPUT);

    printStartupInfo();
}

void loop() {
    unsigned long now = millis();

    if (now - lastDebugPrint >= DEBUG_PRINT_INTERVAL_MS) {
        lastDebugPrint = now;

        Serial.println("ESP32 is running...");

        digitalWrite(ONBOARD_LED_PIN, !digitalRead(ONBOARD_LED_PIN));
    }
}