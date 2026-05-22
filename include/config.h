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

const int ONBOARD_LED_PIN = 2;
const int ONBOARD_BUTTON_PIN = 27;

// ======================================================
// I2C pins
// Used for BME280 sensor
// ======================================================

const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// ======================================================
// Analog sensor pins
// ADC1 pins work while WiFi is active.
// GPIO 34, 35, 36, 39 are input-only pins.
// ======================================================

const int SOIL_MOISTURE_PIN = 34;
const int LDR_PIN = 35;
const int MQ135_PIN = 36;

// ======================================================
// Soil moisture calibration
// These values must be adjusted after testing your sensor.
// Usually: higher value = dry, lower value = wet.
// ======================================================

const int SOIL_DRY_VALUE = 3000;
const int SOIL_WET_VALUE = 1400;

// ======================================================
// Digital output pins
// Used for actuators through relay module or MOSFET.
// Never connect pump or fan directly to ESP32 GPIO.
// ======================================================

const int WATER_PUMP_RELAY_PIN = 26;
const int FAN_RELAY_PIN = 25;

// ======================================================
// Fan relay settings
// ======================================================

// Your relay works as active LOW:
// LOW  = relay ON
// HIGH = relay OFF
const bool FAN_RELAY_ACTIVE_LOW = true;

// Default automatic control settings.
// Frontend can change threshold later through MQTT command.
const float DEFAULT_FAN_THRESHOLD_C = 27.0;

// Difference between ON and OFF temperature.
// Example: threshold 27°C:
// fan ON at 27°C
// fan OFF at 26°C
const float FAN_HYSTERESIS_C = 1.0;

// ======================================================
// Pump relay settings
// ======================================================

// Most relay modules are active LOW:
// LOW  = relay ON
// HIGH = relay OFF
const bool PUMP_RELAY_ACTIVE_LOW = true;

// Default automatic pump control.
// If soil moisture is lower than this value, pump can start automatically.
const int DEFAULT_SOIL_MOISTURE_THRESHOLD_PERCENT = 30;

// Pump safety timer.
// Pump will automatically turn OFF after this time.
const unsigned long PUMP_MAX_ON_TIME_MS = 3000;

// Pump cooldown.
// After automatic watering, wait this time before pump can start automatically again.
const unsigned long PUMP_COOLDOWN_MS = 60000;

// ======================================================
// Timing
// ======================================================

const unsigned long SENSOR_READ_INTERVAL_MS = 5000;
const unsigned long DEBUG_PRINT_INTERVAL_MS = 2000;

// ======================================================
// MQTT settings
// ======================================================

const char* MQTT_BROKER = "mqtt.flespi.io";
const int MQTT_PORT = 1883;

// ESP32 publishes sensor data here
const char* MQTT_TOPIC_SENSOR_DATA =
    "greenhouse/smart-greenhouse/esp32-01/sensor-data";

// ESP32 publishes device status here
const char* MQTT_TOPIC_DEVICE_STATUS =
    "greenhouse/smart-greenhouse/esp32-01/status";

// Backend / Flespi sends pump commands to ESP32 here
const char* MQTT_TOPIC_PUMP_COMMAND =
    "greenhouse/smart-greenhouse/esp32-01/commands/pump";

// Backend / Flespi sends fan commands to ESP32 here
const char* MQTT_TOPIC_FAN_COMMAND =
    "greenhouse/smart-greenhouse/esp32-01/commands/fan";

#endif