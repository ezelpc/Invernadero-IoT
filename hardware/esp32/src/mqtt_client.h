#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>

// Funciones públicas
void initMQTT();
bool connectMQTT();
void disconnectMQTT();
bool publishSensorData(const String& topic, const String& payload);
bool publishMessage(const String& topic, const String& message);
void mqttLoop();
bool isMQTTConnected();
void setActuatorCallback(void (*callback)(String topic, String payload));

#endif // MQTT_CLIENT_H
