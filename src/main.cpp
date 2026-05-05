#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("GreenHouse IoT Firmware started");
    Serial.println("Board: FireBeetle 2 ESP32-E");
    Serial.println("Serial Monitor works");
    Serial.println("=================================");
}

void loop() {
    Serial.println("ESP32 is running...");
    delay(2000);
}