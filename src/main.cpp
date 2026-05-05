#include <Arduino.h>
#include <Wire.h>
#include "config.h"

void identifyDevice(int address) {
    switch (address) {
        case 0x27:
        case 0x3F:
            Serial.print("LCD I2C backpack, often PCF8574");
            break;

        case 0x3C:
        case 0x3D:
            Serial.print("OLED display, often SSD1306");
            break;

        case 0x76:
        case 0x77:
            Serial.print("BME280/BMP280 temperature sensor");
            break;

        case 0x68:
            Serial.print("RTC DS3231 or MPU6050");
            break;

        default:
            Serial.print("Unknown I2C device");
            break;
    }
}

void scanI2CBus() {
    Serial.println();
    Serial.println("Scanning I2C bus...");
    Serial.println("====================");

    int devicesFound = 0;

    for (int address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("Found device at 0x");

            if (address < 16) {
                Serial.print("0");
            }

            Serial.print(address, HEX);
            Serial.print(" - ");
            identifyDevice(address);
            Serial.println();

            devicesFound++;
        }
    }

    Serial.println();

    if (devicesFound == 0) {
        Serial.println("No I2C devices found.");
        Serial.println("Check wiring: 3V3, GND, SDA, SCL.");
    } else {
        Serial.print("Found ");
        Serial.print(devicesFound);
        Serial.println(" I2C device(s).");
    }

    Serial.println("====================");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("GreenHouse IoT Firmware");
    Serial.println("I2C Scanner");
    Serial.println("Board: FireBeetle 2 ESP32-E");
    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);
    Serial.println("=================================");

    Serial.print("Using SDA: GPIO ");
    Serial.println(I2C_SDA_PIN);

    Serial.print("Using SCL: GPIO ");
    Serial.println(I2C_SCL_PIN);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    scanI2CBus();
}

void loop() {
    delay(5000);
    scanI2CBus();
}