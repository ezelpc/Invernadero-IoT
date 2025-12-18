#include "sensors.h"
#include "config.h"
#include <DHT.h>
#include <ArduinoJson.h>

// Instancia del sensor DHT22
DHT dht(PIN_DHT22, DHT_TYPE);

// Variables para filtrado de datos
float lastTemp = 0.0;
float lastHum = 0.0;
float lastSoil = 0.0;
float lastLux = 0.0;

/**
 * Inicializa todos los sensores
 */
void initSensors() {
  DEBUG_PRINTLN("Inicializando sensores...");
  
  // Inicializar DHT22
  dht.begin();
  
  // Configurar pines analógicos
  pinMode(PIN_HUMEDAD_SUELO, INPUT);
  pinMode(PIN_LDR, INPUT);
  
  // Configurar ADC
  analogSetAttenuation(ADC_11db); // Rango completo 0-3.3V
  
  DEBUG_PRINTLN("Sensores inicializados correctamente");
}

/**
 * Lee la temperatura del DHT22 con validación y reintentos
 */
float readTemperature() {
  float temp = NAN;
  
  for (int i = 0; i < SENSOR_RETRY_COUNT; i++) {
    temp = dht.readTemperature();
    
    if (!isnan(temp) && temp >= -40 && temp <= 80) {
      // Filtro simple para evitar cambios bruscos
      if (lastTemp != 0.0 && abs(temp - lastTemp) > 10.0) {
        DEBUG_PRINTLN("Advertencia: Cambio brusco de temperatura detectado");
        delay(SENSOR_RETRY_DELAY_MS);
        continue;
      }
      lastTemp = temp;
      return temp;
    }
    
    DEBUG_PRINTF("Reintento %d de lectura de temperatura\n", i + 1);
    delay(SENSOR_RETRY_DELAY_MS);
  }
  
  DEBUG_PRINTLN("Error: No se pudo leer temperatura");
  return lastTemp; // Retornar última lectura válida
}

/**
 * Lee la humedad del DHT22 con validación y reintentos
 */
float readHumidity() {
  float hum = NAN;
  
  for (int i = 0; i < SENSOR_RETRY_COUNT; i++) {
    hum = dht.readHumidity();
    
    if (!isnan(hum) && hum >= 0 && hum <= 100) {
      // Filtro simple
      if (lastHum != 0.0 && abs(hum - lastHum) > 20.0) {
        DEBUG_PRINTLN("Advertencia: Cambio brusco de humedad detectado");
        delay(SENSOR_RETRY_DELAY_MS);
        continue;
      }
      lastHum = hum;
      return hum;
    }
    
    DEBUG_PRINTF("Reintento %d de lectura de humedad\n", i + 1);
    delay(SENSOR_RETRY_DELAY_MS);
  }
  
  DEBUG_PRINTLN("Error: No se pudo leer humedad");
  return lastHum;
}

/**
 * Lee la humedad del suelo (sensor analógico)
 * Retorna porcentaje: 0% = seco, 100% = húmedo
 */
float readSoilMoisture() {
  const int numReadings = 10;
  long sum = 0;
  
  // Promedio de múltiples lecturas para estabilidad
  for (int i = 0; i < numReadings; i++) {
    sum += analogRead(PIN_HUMEDAD_SUELO);
    delay(10);
  }
  
  int rawValue = sum / numReadings;
  
  // Calibración: ajustar estos valores según tu sensor
  // Valor en aire (seco): ~3000-4095
  // Valor en agua (húmedo): ~1000-1500
  const int dryValue = 3500;
  const int wetValue = 1200;
  
  // Convertir a porcentaje (invertido porque menor valor = más húmedo)
  float moisture = map(rawValue, dryValue, wetValue, 0, 100);
  moisture = constrain(moisture, 0, 100);
  
  lastSoil = moisture;
  return moisture;
}

/**
 * Lee la luminosidad (LDR)
 * Retorna porcentaje: 0% = oscuro, 100% = muy luminoso
 */
float readLuminosity() {
  const int numReadings = 10;
  long sum = 0;
  
  // Promedio de múltiples lecturas
  for (int i = 0; i < numReadings; i++) {
    sum += analogRead(PIN_LDR);
    delay(10);
  }
  
  int rawValue = sum / numReadings;
  
  // Calibración: ajustar según tu LDR
  // Oscuridad total: ~10-100
  // Luz brillante: ~3000-4095
  const int darkValue = 50;
  const int brightValue = 3500;
  
  // Convertir a porcentaje
  float luminosity = map(rawValue, darkValue, brightValue, 0, 100);
  luminosity = constrain(luminosity, 0, 100);
  
  lastLux = luminosity;
  return luminosity;
}

/**
 * Lee todos los sensores y retorna estructura con datos
 */
SensorData readAllSensors() {
  SensorData data;
  
  DEBUG_PRINTLN("\n--- Leyendo sensores ---");
  
  data.temperatura = readTemperature();
  data.humedad = readHumidity();
  data.humedadSuelo = readSoilMoisture();
  data.luminosidad = readLuminosity();
  data.timestamp = millis();
  data.valid = validateSensorData(data);
  
  DEBUG_PRINTF("Temperatura: %.2f °C\n", data.temperatura);
  DEBUG_PRINTF("Humedad: %.2f %%\n", data.humedad);
  DEBUG_PRINTF("Humedad Suelo: %.2f %%\n", data.humedadSuelo);
  DEBUG_PRINTF("Luminosidad: %.2f %%\n", data.luminosidad);
  DEBUG_PRINTF("Válido: %s\n", data.valid ? "Sí" : "No");
  
  return data;
}

/**
 * Valida que los datos de sensores estén en rangos razonables
 */
bool validateSensorData(const SensorData& data) {
  if (isnan(data.temperatura) || data.temperatura < -40 || data.temperatura > 80) {
    DEBUG_PRINTLN("Validación: Temperatura fuera de rango");
    return false;
  }
  
  if (isnan(data.humedad) || data.humedad < 0 || data.humedad > 100) {
    DEBUG_PRINTLN("Validación: Humedad fuera de rango");
    return false;
  }
  
  if (data.humedadSuelo < 0 || data.humedadSuelo > 100) {
    DEBUG_PRINTLN("Validación: Humedad suelo fuera de rango");
    return false;
  }
  
  if (data.luminosidad < 0 || data.luminosidad > 100) {
    DEBUG_PRINTLN("Validación: Luminosidad fuera de rango");
    return false;
  }
  
  return true;
}

/**
 * Convierte datos de sensores a JSON
 */
String sensorDataToJson(const SensorData& data) {
  StaticJsonDocument<256> doc;
  
  doc["thing"] = THING_NAME;
  doc["timestamp"] = data.timestamp;
  doc["temperatura"] = round(data.temperatura * 100) / 100.0;
  doc["humedad"] = round(data.humedad * 100) / 100.0;
  doc["humedadSuelo"] = round(data.humedadSuelo * 100) / 100.0;
  doc["luminosidad"] = round(data.luminosidad * 100) / 100.0;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}
