#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

void setupMqttClient();
bool connectMQTT();
void reconnectMQTTIfNeeded();
void mqttLoop();

bool isMqttConnected();
bool canPublishMqtt();

void publishSensorData();
void publishActuatorStatus(const char* reason);

#endif