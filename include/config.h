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

static constexpr const char* DEVICE_ID = "esp32-01";

// ======================================================
// I2C pins
// Used for BME280 sensor
// ======================================================

static constexpr int I2C_SDA_PIN = 21;
static constexpr int I2C_SCL_PIN = 22;

// ======================================================
// Analog sensor pins
// ADC1 pins work while WiFi is active.
// GPIO 34, 35 are input-only pins.
// ======================================================

static constexpr int SOIL_MOISTURE_PIN = 34;
static constexpr int LDR_PIN = 35;

// ======================================================
// Soil moisture calibration
// Calibrated for this sensor. Higher raw value = dry, lower = wet.
// ======================================================

static constexpr int SOIL_DRY_VALUE = 3000;
static constexpr int SOIL_WET_VALUE = 1400;

// ======================================================
// Digital output pins
// Relay control pins for pump and fan.
// GPIO controls the relay signal only.
// ======================================================

static constexpr int WATER_PUMP_RELAY_PIN = 26;
static constexpr int FAN_RELAY_PIN = 25;

// ======================================================
// Fan relay settings
// ======================================================

// Relay module is active LOW:
// LOW  = relay ON
// HIGH = relay OFF
static constexpr bool FAN_RELAY_ACTIVE_LOW = true;

// Default automatic control settings.
// Frontend can change threshold later through MQTT command.
static constexpr float DEFAULT_FAN_THRESHOLD_C = 27.0;

// Difference between ON and OFF temperature.
// Example: threshold 27°C:
// fan ON at 27°C
// fan OFF at 26°C
static constexpr float FAN_HYSTERESIS_C = 1.0;

// ======================================================
// Pump relay settings
// ======================================================

// Relay module is active LOW:
// LOW  = relay ON
// HIGH = relay OFF
static constexpr bool PUMP_RELAY_ACTIVE_LOW = true;

// Default automatic pump control.
// If soil moisture is lower than this value, pump can start automatically.
static constexpr int DEFAULT_SOIL_MOISTURE_THRESHOLD_PERCENT = 30;

// Pump safety timer.
// Pump will automatically turn OFF after this time.
static constexpr unsigned long PUMP_MAX_ON_TIME_MS = 3000;

// Pump cooldown.
// After automatic watering, wait this time before pump can start automatically again.
static constexpr unsigned long PUMP_COOLDOWN_MS = 60000;

// ======================================================
// System settings
// ======================================================

static constexpr unsigned long SERIAL_BAUD_RATE = 115200;
static constexpr unsigned long STARTUP_DELAY_MS = 1000;

// ======================================================
// ADC settings
// ESP32 ADC is 12-bit: 0-4095
// ======================================================

static constexpr int ADC_RESOLUTION_BITS = 12;
static constexpr int ADC_MAX_VALUE = 4095;

// ======================================================
// Timing
// ======================================================

static constexpr unsigned long SENSOR_READ_INTERVAL_MS = 5000;
static constexpr unsigned long DEBUG_PRINT_INTERVAL_MS = 2000;

// ======================================================
// Reconnect settings
// ======================================================

static constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000;
static constexpr unsigned long WIFI_RECONNECT_INTERVAL_MS = 5000;

// ======================================================
// MQTT buffer settings
// ======================================================

static constexpr int MQTT_CLIENT_BUFFER_SIZE = 1000;
static constexpr int MQTT_STATUS_PAYLOAD_BUFFER_SIZE = 700;
static constexpr int MQTT_SENSOR_PAYLOAD_BUFFER_SIZE = 1000;

// ======================================================
// MQTT settings
// ======================================================

static constexpr const char* MQTT_BROKER = "mqtt.flespi.io";
static constexpr int MQTT_PORT = 1883;

// ESP32 publishes sensor data here
static constexpr const char* MQTT_TOPIC_SENSOR_DATA =
    "greenhouse/smart-greenhouse/esp32-01/sensor-data";

// ESP32 publishes device status here
static constexpr const char* MQTT_TOPIC_DEVICE_STATUS =
    "greenhouse/smart-greenhouse/esp32-01/status";

// Backend / Flespi sends pump commands to ESP32 here
static constexpr const char* MQTT_TOPIC_PUMP_COMMAND =
    "greenhouse/smart-greenhouse/esp32-01/commands/pump";

// Backend / Flespi sends fan commands to ESP32 here
static constexpr const char* MQTT_TOPIC_FAN_COMMAND =
    "greenhouse/smart-greenhouse/esp32-01/commands/fan";

#endif