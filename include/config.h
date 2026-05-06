#ifndef CONFIG_H
#define CONFIG_H

// ======================================================
// GreenHouse IoT Firmware
// Board: FireBeetle 2 ESP32-E
// Main chip: ESP32-E
// Logic level: 3.3V
// ======================================================

// IMPORTANT:
// ESP32 GPIO pins are NOT 5V tolerant.
// Do not connect 5V signals directly to ESP32 GPIO pins.

// ======================================================
// Device information
// ======================================================

const char* DEVICE_ID = "greenhouse-esp32-01";

// ======================================================
// Built-in board components
// ======================================================

// Built-in blue LED on FireBeetle 2 ESP32-E.
// Used only for status/debugging, not as a greenhouse feature.
const int ONBOARD_LED_PIN = 2;

// Built-in user button on FireBeetle 2 ESP32-E.
const int ONBOARD_BUTTON_PIN = 27;

// ======================================================
// I2C pins
// Used for BME280 sensor or I2C LCD display
// ======================================================

const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// ======================================================
// Analog sensor pins
// Use ADC1 pins because they work while WiFi is active.
// GPIO 34, 35, 36, 39 are input-only pins.
// Do NOT use them for relay, pump, fan, LED output.
// ======================================================

// Soil moisture sensor analog output
const int SOIL_MOISTURE_PIN = 34;

// LDR light sensor voltage divider
const int LDR_PIN = 35;

// MQ-135 gas sensor analog output, optional
const int MQ135_PIN = 36;

// ======================================================
// Digital output pins
// Used for actuators through relay module or MOSFET.
// Never connect pump or fan directly to ESP32 GPIO.
// ======================================================

// Relay module for water pump
const int WATER_PUMP_RELAY_PIN = 25;

// Relay module or MOSFET for fan, optional
const int FAN_RELAY_PIN = 26;

// ======================================================
// Timing
// ======================================================

// Read sensors every 5 seconds
const unsigned long SENSOR_READ_INTERVAL_MS = 5000;

// Print debug status every 2 seconds
const unsigned long DEBUG_PRINT_INTERVAL_MS = 2000;

// ======================================================
// MQTT settings
// ======================================================

const char* MQTT_BROKER = "mqtt.flespi.io";
const int MQTT_PORT = 1883;

// ESP32 publishes sensor data here
const char* MQTT_TOPIC_SENSOR_DATA = "greenhouse/smart-greenhouse/esp32-01/sensor-data";

// Future topics for commands/status
const char* MQTT_TOPIC_DEVICE_STATUS = "greenhouse/smart-greenhouse/esp32-01/status";
const char* MQTT_TOPIC_PUMP_COMMAND = "greenhouse/smart-greenhouse/esp32-01/commands/pump";
const char* MQTT_TOPIC_FAN_COMMAND = "greenhouse/smart-greenhouse/esp32-01/commands/fan";

#endif