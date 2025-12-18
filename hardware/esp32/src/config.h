#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// CONFIGURACIÓN WIFI
// ============================================
#define WIFI_SSID "TU_WIFI_SSID"
#define WIFI_PASSWORD "TU_WIFI_PASSWORD"
#define WIFI_TIMEOUT_MS 20000
#define WIFI_RETRY_DELAY_MS 5000

// ============================================
// CONFIGURACIÓN AWS IOT CORE
// ============================================
#define AWS_IOT_ENDPOINT "xxxxxxxxxxxxxx-ats.iot.us-east-1.amazonaws.com"
#define AWS_IOT_PORT 8883
#define THING_NAME "invernadero-01"

// Topics MQTT
#define TOPIC_TEMPERATURA "invernadero/sensores/temperatura"
#define TOPIC_HUMEDAD "invernadero/sensores/humedad"
#define TOPIC_LUMINOSIDAD "invernadero/sensores/luminosidad"
#define TOPIC_HUMEDAD_SUELO "invernadero/sensores/humedad-suelo"
#define TOPIC_ESTADO "invernadero/estado"
#define TOPIC_ALERTAS "invernadero/alertas"
#define TOPIC_ACTUADOR_VENTILADOR "invernadero/actuadores/ventilador"
#define TOPIC_ACTUADOR_BOMBA "invernadero/actuadores/bomba"
#define TOPIC_ACTUADOR_LUCES "invernadero/actuadores/luces"

// ============================================
// CERTIFICADOS AWS IOT
// ============================================
// IMPORTANTE: Reemplazar con tus certificados reales
// Obtener ejecutando: bash scripts/setup-aws.sh

const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
// Certificado CA de Amazon Root CA 1
// Descargar de: https://www.amazontrust.com/repository/AmazonRootCA1.pem
-----END CERTIFICATE-----
)EOF";

const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
// Certificado del dispositivo (thing certificate)
// Generado por AWS IoT Core
-----END CERTIFICATE-----
)EOF";

const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
// Clave privada del dispositivo
// Generado por AWS IoT Core
-----END RSA PRIVATE KEY-----
)EOF";

// ============================================
// CONFIGURACIÓN DE PINES GPIO
// ============================================
// Sensores
#define PIN_DHT22 4                    // GPIO4 - Sensor DHT22
#define PIN_HUMEDAD_SUELO 34           // GPIO34 (ADC1_CH6) - Sensor humedad suelo
#define PIN_LDR 35                     // GPIO35 (ADC1_CH7) - Fotoresistencia

// Actuadores (Relays)
#define PIN_RELAY_VENTILADOR 25        // GPIO25 - Relay ventilador
#define PIN_RELAY_BOMBA 26             // GPIO26 - Relay bomba de riego
#define PIN_RELAY_LUCES 27             // GPIO27 - Relay luces

// LED indicador
#define PIN_LED_STATUS 2               // GPIO2 - LED integrado

// ============================================
// CONFIGURACIÓN DE SENSORES
// ============================================
#define DHT_TYPE DHT22                 // Tipo de sensor DHT
#define SENSOR_READ_INTERVAL_MS 30000  // Leer sensores cada 30 segundos
#define SENSOR_RETRY_COUNT 3           // Reintentos de lectura
#define SENSOR_RETRY_DELAY_MS 2000     // Delay entre reintentos

// ============================================
// UMBRALES Y ALERTAS
// ============================================
// Temperatura (°C)
#define TEMP_MIN 15.0
#define TEMP_MAX 35.0
#define TEMP_IDEAL_MIN 20.0
#define TEMP_IDEAL_MAX 28.0

// Humedad ambiente (%)
#define HUM_MIN 40.0
#define HUM_MAX 80.0
#define HUM_IDEAL_MIN 60.0
#define HUM_IDEAL_MAX 75.0

// Humedad del suelo (%)
#define SOIL_MIN 30.0
#define SOIL_MAX 80.0
#define SOIL_IDEAL_MIN 50.0
#define SOIL_IDEAL_MAX 70.0

// Luminosidad (%)
#define LUX_MIN 20.0
#define LUX_MAX 100.0

// ============================================
// CONFIGURACIÓN MQTT
// ============================================
#define MQTT_BUFFER_SIZE 512
#define MQTT_KEEPALIVE 60
#define MQTT_RECONNECT_DELAY_MS 5000
#define MQTT_MAX_RECONNECT_ATTEMPTS 5

// ============================================
// CONFIGURACIÓN GENERAL
// ============================================
#define SERIAL_BAUD_RATE 115200
#define WATCHDOG_TIMEOUT_S 30
#define ENABLE_SERIAL_DEBUG true

// ============================================
// MACROS DE DEBUG
// ============================================
#if ENABLE_SERIAL_DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, ...)
#endif

#endif // CONFIG_H
