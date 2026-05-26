#include "mqtt_manager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "secrets.h"
#include "sensors.h"
#include "actuators.h"

// ======================================================
// MQTT objects/state
// ======================================================

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastMqttReconnectAttempt = 0;

// ======================================================
// MQTT helper functions
// ======================================================

bool isMqttConnected() {
    return mqttClient.connected();
}

bool canPublishMqtt() {
    return WiFi.status() == WL_CONNECTED && mqttClient.connected();
}

// ======================================================
// Publish actuator/device status
// This goes to MQTT_TOPIC_DEVICE_STATUS
// ======================================================

void publishActuatorStatus(const char* reason) {
    if (!canPublishMqtt()) {
        Serial.println("MQTT not connected. Skipping actuator status publish.");
        return;
    }

    StaticJsonDocument<700> doc;

    doc["deviceId"] = DEVICE_ID;
    doc["online"] = true;
    doc["status"] = "ONLINE";
    doc["reason"] = reason;

    doc["fanOn"] = isFanOn();
    doc["fanState"] = toOnOff(isFanOn());
    doc["fanAutoMode"] = isFanAutoMode();
    doc["fanMode"] = toMode(isFanAutoMode());
    doc["fanThreshold"] = getFanThreshold();

    doc["pumpOn"] = isPumpOn();
    doc["pumpState"] = toOnOff(isPumpOn());
    doc["pumpAutoMode"] = isPumpAutoMode();
    doc["pumpMode"] = toMode(isPumpAutoMode());
    doc["soilMoistureThreshold"] = getSoilMoistureThreshold();

    char mqttPayload[700];
    serializeJson(doc, mqttPayload, sizeof(mqttPayload));

    bool success = mqttClient.publish(
        MQTT_TOPIC_DEVICE_STATUS,
        mqttPayload,
        true
    );

    Serial.print("Device status payload: ");
    Serial.println(mqttPayload);

    Serial.print("Device status publish: ");
    Serial.println(success ? "success" : "failed");
}

// ======================================================
// MQTT callback
// ======================================================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";

    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    message.trim();

    Serial.println();
    Serial.print("MQTT command topic: ");
    Serial.println(topic);

    Serial.print("MQTT command payload: ");
    Serial.println(message);

    if (String(topic) == MQTT_TOPIC_FAN_COMMAND) {
        handleFanCommand(message);
        publishActuatorStatus("fan-command");
        publishSensorData();
        return;
    }

    if (String(topic) == MQTT_TOPIC_PUMP_COMMAND) {
        handlePumpCommand(message);
        publishActuatorStatus("pump-command");
        publishSensorData();
        return;
    }

    Serial.println("Unknown MQTT topic.");
}

// ======================================================
// MQTT setup / connection
// ======================================================

void setupMqttClient() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setBufferSize(1000);
    mqttClient.setCallback(mqttCallback);
}

bool connectMQTT() {
    if (mqttClient.connected()) {
        return true;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot connect MQTT because WiFi is disconnected.");
        return false;
    }

    Serial.print("Connecting to MQTT broker: ");
    Serial.println(MQTT_BROKER);

    String clientId = "greenhouse-esp32-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    String offlinePayload = "{\"deviceId\":\"";
    offlinePayload += DEVICE_ID;
    offlinePayload += "\",\"online\":false,\"status\":\"OFFLINE\"}";

    bool connected = mqttClient.connect(
        clientId.c_str(),
        MQTT_TOKEN,
        "",
        MQTT_TOPIC_DEVICE_STATUS,
        1,
        true,
        offlinePayload.c_str()
    );

    if (connected) {
        Serial.println("MQTT connected!");

        mqttClient.subscribe(MQTT_TOPIC_FAN_COMMAND);
        mqttClient.subscribe(MQTT_TOPIC_PUMP_COMMAND);

        Serial.print("Subscribed to fan command topic: ");
        Serial.println(MQTT_TOPIC_FAN_COMMAND);

        Serial.print("Subscribed to pump command topic: ");
        Serial.println(MQTT_TOPIC_PUMP_COMMAND);

        publishActuatorStatus("mqtt-connected");

        return true;
    }

    Serial.print("MQTT connection failed. State: ");
    Serial.println(mqttClient.state());

    return false;
}

void reconnectMQTTIfNeeded() {
    if (mqttClient.connected()) {
        return;
    }

    unsigned long now = millis();

    if (now - lastMqttReconnectAttempt >= 5000) {
        lastMqttReconnectAttempt = now;
        connectMQTT();
    }
}

void mqttLoop() {
    mqttClient.loop();
}

// ======================================================
// Publish sensor data
// This goes to MQTT_TOPIC_SENSOR_DATA
// ======================================================

void publishSensorData() {
    if (!isSensorReady()) {
        Serial.println("Sensor not connected. Skipping MQTT publish.");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected. Skipping MQTT publish.");
        return;
    }

    if (!mqttClient.connected()) {
        Serial.println("MQTT disconnected. Skipping MQTT publish.");
        return;
    }

    SensorData data = readSensorData();

    updateFanAutomatic(data.temperature);
    updatePumpAutomatic(data.soilMoisturePercent);

    StaticJsonDocument<1000> doc;

    doc["deviceId"] = DEVICE_ID;
    doc["online"] = true;

    doc["temperature"] = data.temperature;
    doc["humidity"] = data.humidity;
    doc["pressure"] = data.pressure;
    doc["altitude"] = data.altitude;

    doc["soilMoistureRaw"] = data.soilRaw;
    doc["soilMoisture"] = data.soilMoisturePercent;

    doc["fanOn"] = isFanOn();
    doc["fanState"] = toOnOff(isFanOn());
    doc["fanAutoMode"] = isFanAutoMode();
    doc["fanMode"] = toMode(isFanAutoMode());
    doc["fanThreshold"] = getFanThreshold();

    doc["pumpOn"] = isPumpOn();
    doc["pumpState"] = toOnOff(isPumpOn());
    doc["pumpAutoMode"] = isPumpAutoMode();
    doc["pumpMode"] = toMode(isPumpAutoMode());
    doc["soilMoistureThreshold"] = getSoilMoistureThreshold();

    char mqttPayload[1000];
    serializeJson(doc, mqttPayload, sizeof(mqttPayload));

    bool success = mqttClient.publish(MQTT_TOPIC_SENSOR_DATA, mqttPayload);

    Serial.println();
    Serial.println("Sensor readings");
    Serial.println("----------------------");

    Serial.print("Temperature: ");
    Serial.print(data.temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(data.humidity);
    Serial.println(" %");

    Serial.print("Pressure: ");
    Serial.print(data.pressure);
    Serial.println(" hPa");

    Serial.print("Altitude: ");
    Serial.print(data.altitude);
    Serial.println(" m");

    Serial.print("Soil raw: ");
    Serial.println(data.soilRaw);

    Serial.print("Soil moisture: ");
    Serial.print(data.soilMoisturePercent);
    Serial.println(" %");

    Serial.print("Fan: ");
    Serial.println(isFanOn() ? "ON" : "OFF");

    Serial.print("Fan mode: ");
    Serial.println(isFanAutoMode() ? "AUTO" : "MANUAL");

    Serial.print("Fan threshold: ");
    Serial.print(getFanThreshold());
    Serial.println(" °C");

    Serial.print("Pump: ");
    Serial.println(isPumpOn() ? "ON" : "OFF");

    Serial.print("Pump mode: ");
    Serial.println(isPumpAutoMode() ? "AUTO" : "MANUAL");

    Serial.print("Soil threshold: ");
    Serial.print(getSoilMoistureThreshold());
    Serial.println(" %");

    Serial.print("MQTT payload: ");
    Serial.println(mqttPayload);

    Serial.print("Publish status: ");
    Serial.println(success ? "success" : "failed");
}