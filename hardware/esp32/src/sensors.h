#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// Estructura para datos de sensores
struct SensorData {
  float temperatura;
  float humedad;
  float humedadSuelo;
  float luminosidad;
  bool valid;
  unsigned long timestamp;
};

// Funciones públicas
void initSensors();
SensorData readAllSensors();
float readTemperature();
float readHumidity();
float readSoilMoisture();
float readLuminosity();
bool validateSensorData(const SensorData& data);
String sensorDataToJson(const SensorData& data);

#endif // SENSORS_H
