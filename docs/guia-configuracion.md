# Guía de Configuración - Invernadero IoT

## Requisitos Previos

### Hardware
- ESP32 DevKit v1
- Sensor DHT22 
- Sensor de humedad de suelo (analógico)
- LDR (fotoresistencia) + resistencia 10kΩ
- 3x Módulos Relay 5V
- Ventilador, bomba, luces (12V)
- Fuentes de alimentación (5V 2A, 12V 3A)
- Cables jumper, protoboard

### Software
- [PlatformIO](https://platformio.org/) o PlatformIO IDE (VS Code)
- [AWS CLI](https://aws.amazon.com/cli/)
- [Node.js 18+](https://nodejs.org/)
- [Git](https://git-scm.com/)

### Cuenta AWS
- Cuenta AWS activa
- Permisos para: IoT Core, Lambda, DynamoDB, S3, CloudFormation, IAM

## Paso 1: Clonar Repositorio

```bash
git clone https://github.com/tu-usuario/Invernadero-IoT.git
cd Invernadero-IoT
```

## Paso 2: Configurar AWS

### 2.1 Configurar Credenciales

```bash
aws configure
```

Ingresar:
- AWS Access Key ID
- AWS Secret Access Key
- Región (recomendado: `us-east-1`)
- Output format: `json`

### 2.2 Ejecutar Script de Setup

```bash
bash scripts/setup-aws.sh
```

Este script:
- ✅ Crea IoT Thing (`invernadero-01`)
- ✅ Genera certificados X.509
- ✅ Crea y adjunta política IoT
- ✅ Despliega stack CloudFormation
- ✅ Configura reglas IoT

**Salida importante**:
```
IoT Endpoint: xxxxxx-ats.iot.us-east-1.amazonaws.com
Certificados guardados en: ./certs/
```

## Paso 3: Configurar ESP32

### 3.1 Copiar Certificados

Abrir `hardware/esp32/src/config.h` y reemplazar:

```cpp
// WiFi
#define WIFI_SSID "TU_RED_WIFI"
#define WIFI_PASSWORD "TU_PASSWORD"

// AWS IoT Endpoint (del paso 2.2)
#define AWS_IOT_ENDPOINT "xxxxxx-ats.iot.us-east-1.amazonaws.com"
```

Copiar contenido de los certificados:

```cpp
const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
// Contenido de certs/AmazonRootCA1.pem
-----END CERTIFICATE-----
)EOF";

const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
// Contenido de certs/device-certificate.pem.crt
-----END CERTIFICATE-----
)EOF";

const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
// Contenido de certs/private-key.pem.key
-----END RSA PRIVATE KEY-----
)EOF";
```

### 3.2 Calibrar Sensores (Opcional)

En `hardware/esp32/src/sensors.cpp`, ajustar valores según tus sensores:

```cpp
// Sensor de humedad de suelo
const int dryValue = 3500;  // Medir en aire
const int wetValue = 1200;  // Medir en agua

// LDR
const int darkValue = 50;    // Medir en oscuridad
const int brightValue = 3500; // Medir con luz brillante
```

### 3.3 Compilar y Cargar Firmware

```bash
cd hardware/esp32

# Compilar
pio run

# Cargar al ESP32 (conectar vía USB)
pio run --target upload

# Abrir monitor serial
pio device monitor
```

**Salida esperada**:
```
Conectando a WiFi: TU_RED_WIFI
¡WiFi conectado!
Dirección IP: 192.168.1.100
Conectando a AWS IoT Core... ¡Conectado!
Sistema inicializado correctamente
```

## Paso 4: Verificar Funcionamiento

### 4.1 Monitorear Mensajes MQTT

En AWS Console → IoT Core → Test:

Suscribirse a: `invernadero/#`

Deberías ver mensajes cada 30 segundos:
```json
{
  "thing": "invernadero-01",
  "timestamp": 1234567890,
  "temperatura": 25.5,
  "humedad": 65.2,
  "humedadSuelo": 55.0,
  "luminosidad": 75.3
}
```

### 4.2 Verificar DynamoDB

```bash
aws dynamodb scan \
  --table-name invernadero-iot-sensor-data-prod \
  --max-items 5
```

### 4.3 Verificar Logs Lambda

```bash
aws logs tail /aws/lambda/invernadero-iot-process-sensor-data-prod --follow
```

## Paso 5: Configurar Dashboard Web

### 5.1 Editar Configuración

Abrir `dashboard/web/app.js` y actualizar:

```javascript
const CONFIG = {
  region: 'us-east-1',
  iotEndpoint: 'xxxxxx-ats.iot.us-east-1.amazonaws.com',
  accessKeyId: 'TU_ACCESS_KEY_ID',
  secretAccessKey: 'TU_SECRET_ACCESS_KEY',
  // ...
};
```

> ⚠️ **Seguridad**: Para producción, usar AWS Cognito en lugar de credenciales hardcodeadas.

### 5.2 Abrir Dashboard

```bash
# Abrir en navegador
open dashboard/web/index.html

# O usar servidor local
python -m http.server 8000
# Navegar a http://localhost:8000/dashboard/web/
```

### 5.3 Modo Demo (Sin AWS)

Para probar sin AWS, descomentar en `app.js`:

```javascript
// startDemoMode();
```

## Paso 6: Configurar Grafana Cloud (Opcional)

### 6.1 Crear Cuenta Grafana Cloud

1. Ir a [grafana.com](https://grafana.com)
2. Crear cuenta gratuita
3. Crear stack

### 6.2 Configurar Data Source

1. Settings → Data Sources → Add data source
2. Seleccionar "Amazon DynamoDB"
3. Configurar:
   - Region: `us-east-1`
   - Access Key ID / Secret Access Key
   - Default Table: `invernadero-iot-sensor-data-prod`

### 6.3 Importar Dashboard

1. Dashboards → Import
2. Upload `dashboard/grafana/dashboards/invernadero.json`
3. Seleccionar data source configurado

## Paso 7: Probar Control de Actuadores

### 7.1 Desde Dashboard Web

Usar los switches en la sección "Control de Actuadores"

### 7.2 Desde AWS IoT Test

Publicar a topic `invernadero/actuadores/ventilador`:

```json
{
  "state": "on"
}
```

### 7.3 Verificar en Monitor Serial

```
--- Comando de actuador recibido ---
Topic: invernadero/actuadores/ventilador
Payload: {"state":"on"}
Ventilador ENCENDIDO
```

## Troubleshooting

### ESP32 no conecta a WiFi
- Verificar SSID y password
- Verificar que el router soporte 2.4GHz
- Revisar intensidad de señal (>-70 dBm)

### MQTT no conecta
- Verificar endpoint AWS IoT
- Verificar certificados (formato PEM correcto)
- Revisar política IoT en AWS Console
- Verificar logs en CloudWatch

### Sensores devuelven NaN
- Verificar conexiones físicas
- Verificar alimentación de sensores
- Revisar pines GPIO en `config.h`
- Aumentar `SENSOR_RETRY_COUNT`

### Lambda no se ejecuta
- Verificar regla IoT está habilitada
- Revisar permisos IAM del rol Lambda
- Verificar logs en CloudWatch
- Validar formato JSON de mensajes MQTT

## Configuración Avanzada

### Cambiar Intervalo de Lectura

En `config.h`:
```cpp
#define SENSOR_READ_INTERVAL_MS 60000  // 60 segundos
```

### Ajustar Umbrales

En `config.h`:
```cpp
#define TEMP_MAX 30.0  // Temperatura máxima
#define SOIL_MIN 40.0  // Humedad suelo mínima
```

### Habilitar/Deshabilitar Debug

En `config.h`:
```cpp
#define ENABLE_SERIAL_DEBUG false  // Deshabilitar para producción
```

## Siguiente Pasos

1. ✅ Montar hardware en invernadero real
2. ✅ Configurar alertas SNS (email/SMS)
3. ✅ Implementar API Gateway para dashboard
4. ✅ Configurar Cognito para autenticación
5. ✅ Agregar más sensores (CO2, pH, etc.)

## Recursos Adicionales

- [Documentación AWS IoT Core](https://docs.aws.amazon.com/iot/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [PlatformIO Docs](https://docs.platformio.org/)
- [Grafana Docs](https://grafana.com/docs/)

## Soporte

Para problemas o preguntas:
- Abrir issue en GitHub
- Revisar logs en CloudWatch
- Consultar documentación oficial de AWS
