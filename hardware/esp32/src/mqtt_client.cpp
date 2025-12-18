#include "mqtt_client.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Clientes WiFi y MQTT
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// Callback para actuadores
void (*actuatorCallbackFunction)(String topic, String payload) = nullptr;

// Variables de estado
unsigned long lastReconnectAttempt = 0;
int reconnectAttempts = 0;

/**
 * Callback interno de MQTT para mensajes recibidos
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  DEBUG_PRINT("Mensaje recibido en topic: ");
  DEBUG_PRINTLN(topic);
  
  // Convertir payload a String
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  DEBUG_PRINT("Payload: ");
  DEBUG_PRINTLN(message);
  
  // Llamar al callback de actuadores si está definido
  if (actuatorCallbackFunction != nullptr) {
    actuatorCallbackFunction(String(topic), message);
  }
}

/**
 * Inicializa el cliente MQTT
 */
void initMQTT() {
  DEBUG_PRINTLN("Inicializando cliente MQTT...");
  
  // Configurar certificados SSL/TLS
  wifiClient.setCACert(AWS_CERT_CA);
  wifiClient.setCertificate(AWS_CERT_CRT);
  wifiClient.setPrivateKey(AWS_CERT_PRIVATE);
  
  // Configurar servidor MQTT
  mqttClient.setServer(AWS_IOT_ENDPOINT, AWS_IOT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);
  
  DEBUG_PRINTLN("Cliente MQTT inicializado");
}

/**
 * Conecta al broker MQTT de AWS IoT Core
 */
bool connectMQTT() {
  if (mqttClient.connected()) {
    return true;
  }
  
  DEBUG_PRINT("Conectando a AWS IoT Core...");
  
  // Intentar conexión
  if (mqttClient.connect(THING_NAME)) {
    DEBUG_PRINTLN(" ¡Conectado!");
    
    // Suscribirse a topics de actuadores
    mqttClient.subscribe(TOPIC_ACTUADOR_VENTILADOR);
    mqttClient.subscribe(TOPIC_ACTUADOR_BOMBA);
    mqttClient.subscribe(TOPIC_ACTUADOR_LUCES);
    
    DEBUG_PRINTLN("Suscrito a topics de actuadores");
    
    // Publicar mensaje de estado
    String statusMsg = "{\"thing\":\"" + String(THING_NAME) + "\",\"status\":\"online\",\"timestamp\":" + String(millis()) + "}";
    mqttClient.publish(TOPIC_ESTADO, statusMsg.c_str());
    
    reconnectAttempts = 0;
    return true;
  } else {
    DEBUG_PRINT(" Error de conexión, rc=");
    DEBUG_PRINTLN(mqttClient.state());
    
    reconnectAttempts++;
    
    if (reconnectAttempts >= MQTT_MAX_RECONNECT_ATTEMPTS) {
      DEBUG_PRINTLN("Máximo de reintentos alcanzado. Reiniciando ESP32...");
      delay(1000);
      ESP.restart();
    }
    
    return false;
  }
}

/**
 * Desconecta del broker MQTT
 */
void disconnectMQTT() {
  if (mqttClient.connected()) {
    // Publicar mensaje de desconexión
    String statusMsg = "{\"thing\":\"" + String(THING_NAME) + "\",\"status\":\"offline\",\"timestamp\":" + String(millis()) + "}";
    mqttClient.publish(TOPIC_ESTADO, statusMsg.c_str());
    
    mqttClient.disconnect();
    DEBUG_PRINTLN("Desconectado de MQTT");
  }
}

/**
 * Publica datos de sensores a un topic
 */
bool publishSensorData(const String& topic, const String& payload) {
  if (!mqttClient.connected()) {
    DEBUG_PRINTLN("Error: MQTT no conectado");
    return false;
  }
  
  bool success = mqttClient.publish(topic.c_str(), payload.c_str());
  
  if (success) {
    DEBUG_PRINT("Publicado en ");
    DEBUG_PRINT(topic);
    DEBUG_PRINT(": ");
    DEBUG_PRINTLN(payload);
  } else {
    DEBUG_PRINT("Error al publicar en ");
    DEBUG_PRINTLN(topic);
  }
  
  return success;
}

/**
 * Publica un mensaje genérico
 */
bool publishMessage(const String& topic, const String& message) {
  return publishSensorData(topic, message);
}

/**
 * Mantiene la conexión MQTT activa
 * Debe llamarse en el loop principal
 */
void mqttLoop() {
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    
    if (now - lastReconnectAttempt > MQTT_RECONNECT_DELAY_MS) {
      lastReconnectAttempt = now;
      
      DEBUG_PRINTLN("Intentando reconectar MQTT...");
      connectMQTT();
    }
  } else {
    mqttClient.loop();
  }
}

/**
 * Verifica si MQTT está conectado
 */
bool isMQTTConnected() {
  return mqttClient.connected();
}

/**
 * Establece el callback para comandos de actuadores
 */
void setActuatorCallback(void (*callback)(String topic, String payload)) {
  actuatorCallbackFunction = callback;
}
