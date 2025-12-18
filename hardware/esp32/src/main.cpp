#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensors.h"
#include "mqtt_client.h"

// Variables globales
unsigned long lastSensorRead = 0;
bool systemInitialized = false;

// Estados de actuadores
bool ventiladorState = false;
bool bombaState = false;
bool lucesState = false;

/**
 * Inicializa la conexión WiFi
 */
void initWiFi() {
  DEBUG_PRINTLN("\n=================================");
  DEBUG_PRINTLN("Sistema de Monitoreo Invernadero");
  DEBUG_PRINTLN("=================================\n");
  
  DEBUG_PRINT("Conectando a WiFi: ");
  DEBUG_PRINTLN(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && 
         millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
    delay(500);
    DEBUG_PRINT(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN("\n¡WiFi conectado!");
    DEBUG_PRINT("Dirección IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    DEBUG_PRINT("Intensidad señal: ");
    DEBUG_PRINT(WiFi.RSSI());
    DEBUG_PRINTLN(" dBm");
  } else {
    DEBUG_PRINTLN("\nError: No se pudo conectar a WiFi");
    DEBUG_PRINTLN("Reiniciando en 5 segundos...");
    delay(5000);
    ESP.restart();
  }
}

/**
 * Inicializa los pines de actuadores
 */
void initActuators() {
  DEBUG_PRINTLN("Inicializando actuadores...");
  
  pinMode(PIN_RELAY_VENTILADOR, OUTPUT);
  pinMode(PIN_RELAY_BOMBA, OUTPUT);
  pinMode(PIN_RELAY_LUCES, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);
  
  // Apagar todos los relays al inicio (relays activos en LOW)
  digitalWrite(PIN_RELAY_VENTILADOR, HIGH);
  digitalWrite(PIN_RELAY_BOMBA, HIGH);
  digitalWrite(PIN_RELAY_LUCES, HIGH);
  digitalWrite(PIN_LED_STATUS, LOW);
  
  DEBUG_PRINTLN("Actuadores inicializados (todos apagados)");
}

/**
 * Controla un actuador
 */
void controlActuator(int pin, bool state, const char* name) {
  // Los relays suelen ser activos en LOW
  digitalWrite(pin, state ? LOW : HIGH);
  
  DEBUG_PRINT(name);
  DEBUG_PRINTLN(state ? " ENCENDIDO" : " APAGADO");
}

/**
 * Callback para comandos de actuadores recibidos por MQTT
 */
void handleActuatorCommand(String topic, String payload) {
  DEBUG_PRINTLN("\n--- Comando de actuador recibido ---");
  DEBUG_PRINT("Topic: ");
  DEBUG_PRINTLN(topic);
  DEBUG_PRINT("Payload: ");
  DEBUG_PRINTLN(payload);
  
  // Parsear JSON
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    DEBUG_PRINT("Error al parsear JSON: ");
    DEBUG_PRINTLN(error.c_str());
    return;
  }
  
  // Obtener estado (on/off o true/false)
  bool state = false;
  if (doc.containsKey("state")) {
    String stateStr = doc["state"].as<String>();
    state = (stateStr == "on" || stateStr == "ON" || stateStr == "true" || stateStr == "1");
  } else if (doc.containsKey("value")) {
    state = doc["value"].as<bool>();
  }
  
  // Controlar actuador según topic
  if (topic == TOPIC_ACTUADOR_VENTILADOR) {
    ventiladorState = state;
    controlActuator(PIN_RELAY_VENTILADOR, state, "Ventilador");
  } 
  else if (topic == TOPIC_ACTUADOR_BOMBA) {
    bombaState = state;
    controlActuator(PIN_RELAY_BOMBA, state, "Bomba");
  } 
  else if (topic == TOPIC_ACTUADOR_LUCES) {
    lucesState = state;
    controlActuator(PIN_RELAY_LUCES, state, "Luces");
  }
  
  // Confirmar estado con LED
  digitalWrite(PIN_LED_STATUS, ventiladorState || bombaState || lucesState ? HIGH : LOW);
}

/**
 * Verifica umbrales y genera alertas
 */
void checkThresholdsAndAlert(const SensorData& data) {
  StaticJsonDocument<256> alertDoc;
  bool alertGenerated = false;
  
  alertDoc["thing"] = THING_NAME;
  alertDoc["timestamp"] = data.timestamp;
  
  JsonArray alerts = alertDoc.createNestedArray("alerts");
  
  // Verificar temperatura
  if (data.temperatura < TEMP_MIN) {
    JsonObject alert = alerts.createNestedObject();
    alert["type"] = "temperatura";
    alert["severity"] = "warning";
    alert["message"] = "Temperatura muy baja";
    alert["value"] = data.temperatura;
    alertGenerated = true;
  } else if (data.temperatura > TEMP_MAX) {
    JsonObject alert = alerts.createNestedObject();
    alert["type"] = "temperatura";
    alert["severity"] = "critical";
    alert["message"] = "Temperatura muy alta";
    alert["value"] = data.temperatura;
    alertGenerated = true;
    
    // Auto-activar ventilador si temperatura muy alta
    if (!ventiladorState) {
      DEBUG_PRINTLN("Auto-activando ventilador por temperatura alta");
      ventiladorState = true;
      controlActuator(PIN_RELAY_VENTILADOR, true, "Ventilador");
    }
  }
  
  // Verificar humedad del suelo
  if (data.humedadSuelo < SOIL_MIN) {
    JsonObject alert = alerts.createNestedObject();
    alert["type"] = "humedad_suelo";
    alert["severity"] = "warning";
    alert["message"] = "Suelo muy seco";
    alert["value"] = data.humedadSuelo;
    alertGenerated = true;
    
    // Auto-activar bomba si suelo muy seco
    if (!bombaState) {
      DEBUG_PRINTLN("Auto-activando bomba por suelo seco");
      bombaState = true;
      controlActuator(PIN_RELAY_BOMBA, true, "Bomba");
    }
  }
  
  // Verificar luminosidad
  if (data.luminosidad < LUX_MIN) {
    JsonObject alert = alerts.createNestedObject();
    alert["type"] = "luminosidad";
    alert["severity"] = "info";
    alert["message"] = "Poca luz detectada";
    alert["value"] = data.luminosidad;
    alertGenerated = true;
  }
  
  // Publicar alertas si se generaron
  if (alertGenerated) {
    String alertJson;
    serializeJson(alertDoc, alertJson);
    publishMessage(TOPIC_ALERTAS, alertJson);
  }
}

/**
 * Setup inicial
 */
void setup() {
  // Inicializar serial
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  // Inicializar WiFi
  initWiFi();
  
  // Inicializar sensores
  initSensors();
  
  // Inicializar actuadores
  initActuators();
  
  // Inicializar MQTT
  initMQTT();
  setActuatorCallback(handleActuatorCommand);
  
  // Conectar a MQTT
  if (connectMQTT()) {
    DEBUG_PRINTLN("\n¡Sistema inicializado correctamente!");
    systemInitialized = true;
    
    // Parpadear LED para indicar inicialización exitosa
    for (int i = 0; i < 3; i++) {
      digitalWrite(PIN_LED_STATUS, HIGH);
      delay(200);
      digitalWrite(PIN_LED_STATUS, LOW);
      delay(200);
    }
  } else {
    DEBUG_PRINTLN("\nError: No se pudo conectar a MQTT");
  }
  
  DEBUG_PRINTLN("\n=================================");
  DEBUG_PRINTLN("Iniciando monitoreo...");
  DEBUG_PRINTLN("=================================\n");
}

/**
 * Loop principal
 */
void loop() {
  // Mantener conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("WiFi desconectado. Reconectando...");
    initWiFi();
  }
  
  // Mantener conexión MQTT
  mqttLoop();
  
  // Leer sensores según intervalo configurado
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
    lastSensorRead = currentMillis;
    
    // Leer todos los sensores
    SensorData data = readAllSensors();
    
    if (data.valid && isMQTTConnected()) {
      // Convertir a JSON
      String jsonData = sensorDataToJson(data);
      
      // Publicar datos completos
      publishSensorData(TOPIC_TEMPERATURA, jsonData);
      publishSensorData(TOPIC_HUMEDAD, jsonData);
      publishSensorData(TOPIC_HUMEDAD_SUELO, jsonData);
      publishSensorData(TOPIC_LUMINOSIDAD, jsonData);
      
      // Verificar umbrales y generar alertas
      checkThresholdsAndAlert(data);
      
      // Parpadear LED para indicar transmisión
      digitalWrite(PIN_LED_STATUS, HIGH);
      delay(100);
      digitalWrite(PIN_LED_STATUS, LOW);
      
    } else if (!data.valid) {
      DEBUG_PRINTLN("Advertencia: Datos de sensores no válidos, no se publicarán");
    } else {
      DEBUG_PRINTLN("Advertencia: MQTT no conectado, no se publicarán datos");
    }
  }
  
  // Pequeño delay para no saturar el CPU
  delay(100);
}
