# Documentación API MQTT - Invernadero IoT

## Descripción General

El sistema utiliza MQTT como protocolo de comunicación entre el ESP32 y AWS IoT Core. Todos los mensajes están en formato JSON y utilizan TLS 1.2 para seguridad.

## Endpoint

```
xxxxxx-ats.iot.us-east-1.amazonaws.com:8883
```

## Autenticación

- **Protocolo**: MQTT sobre TLS 1.2
- **Autenticación**: Certificados X.509
- **Certificados requeridos**:
  - Amazon Root CA 1
  - Device Certificate
  - Private Key

## Topics MQTT

### Topics de Sensores (Publicación desde ESP32)

#### `invernadero/sensores/temperatura`

**Dirección**: ESP32 → AWS IoT Core  
**QoS**: 1  
**Frecuencia**: Cada 30 segundos

**Payload**:
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

**Campos**:
- `thing` (string): Nombre del dispositivo IoT
- `timestamp` (number): Timestamp en milisegundos desde epoch
- `temperatura` (number): Temperatura en °C (rango: -40 a 80)
- `humedad` (number): Humedad ambiente en % (rango: 0 a 100)
- `humedadSuelo` (number): Humedad del suelo en % (rango: 0 a 100)
- `luminosidad` (number): Luminosidad en % (rango: 0 a 100)

**Ejemplo**:
```json
{
  "thing": "invernadero-01",
  "timestamp": 1703001234567,
  "temperatura": 24.3,
  "humedad": 68.5,
  "humedadSuelo": 52.1,
  "luminosidad": 78.9
}
```

---

#### `invernadero/sensores/humedad`

Mismo formato que `temperatura`. Todos los sensores publican al mismo payload completo.

---

#### `invernadero/sensores/luminosidad`

Mismo formato que `temperatura`.

---

#### `invernadero/sensores/humedad-suelo`

Mismo formato que `temperatura`.

---

### Topics de Actuadores (Suscripción en ESP32)

#### `invernadero/actuadores/ventilador`

**Dirección**: AWS IoT Core → ESP32  
**QoS**: 1

**Payload**:
```json
{
  "state": "on",
  "timestamp": 1234567890
}
```

**Campos**:
- `state` (string): Estado del actuador. Valores: `"on"`, `"off"`, `"ON"`, `"OFF"`
- `timestamp` (number, opcional): Timestamp del comando

**Ejemplo - Encender**:
```json
{
  "state": "on",
  "timestamp": 1703001234567
}
```

**Ejemplo - Apagar**:
```json
{
  "state": "off"
}
```

**Respuesta ESP32**: No hay respuesta explícita, pero el cambio se refleja en el próximo mensaje de estado.

---

#### `invernadero/actuadores/bomba`

**Dirección**: AWS IoT Core → ESP32  
**QoS**: 1

Mismo formato que `ventilador`.

**Ejemplo**:
```json
{
  "state": "on"
}
```

---

#### `invernadero/actuadores/luces`

**Dirección**: AWS IoT Core → ESP32  
**QoS**: 1

Mismo formato que `ventilador`.

---

### Topics de Sistema

#### `invernadero/estado`

**Dirección**: ESP32 → AWS IoT Core  
**QoS**: 1  
**Frecuencia**: Al conectar/desconectar

**Payload**:
```json
{
  "thing": "invernadero-01",
  "status": "online",
  "timestamp": 1234567890
}
```

**Campos**:
- `thing` (string): Nombre del dispositivo
- `status` (string): Estado de conexión. Valores: `"online"`, `"offline"`
- `timestamp` (number): Timestamp del evento

**Ejemplo - Conexión**:
```json
{
  "thing": "invernadero-01",
  "status": "online",
  "timestamp": 1703001234567
}
```

**Ejemplo - Desconexión**:
```json
{
  "thing": "invernadero-01",
  "status": "offline",
  "timestamp": 1703001234567
}
```

---

#### `invernadero/alertas`

**Dirección**: Lambda → AWS IoT Core → ESP32  
**QoS**: 1  
**Frecuencia**: Cuando se exceden umbrales

**Payload**:
```json
{
  "thing": "invernadero-01",
  "timestamp": 1234567890,
  "alerts": [
    {
      "type": "temperatura",
      "severity": "critical",
      "message": "Temperatura crítica: 38.5°C",
      "value": 38.5,
      "threshold": 35,
      "action": "Activar ventilador inmediatamente"
    }
  ]
}
```

**Campos**:
- `thing` (string): Nombre del dispositivo
- `timestamp` (number): Timestamp de la alerta
- `alerts` (array): Array de objetos de alerta

**Objeto Alert**:
- `type` (string): Tipo de sensor. Valores: `"temperatura"`, `"humedad"`, `"humedad_suelo"`, `"luminosidad"`
- `severity` (string): Severidad. Valores: `"info"`, `"warning"`, `"critical"`
- `message` (string): Mensaje descriptivo
- `value` (number): Valor actual del sensor
- `threshold` (number): Umbral excedido
- `action` (string, opcional): Acción recomendada

**Ejemplo - Múltiples alertas**:
```json
{
  "thing": "invernadero-01",
  "timestamp": 1703001234567,
  "alerts": [
    {
      "type": "temperatura",
      "severity": "critical",
      "message": "Temperatura crítica: 38.5°C",
      "value": 38.5,
      "threshold": 35,
      "action": "Activar ventilador inmediatamente"
    },
    {
      "type": "humedad_suelo",
      "severity": "warning",
      "message": "Suelo seco: 28.3%",
      "value": 28.3,
      "threshold": 30
    }
  ]
}
```

---

### Device Shadow

#### `$aws/things/invernadero-01/shadow/update`

**Dirección**: Bidireccional  
**QoS**: 1

**Payload - Desired State**:
```json
{
  "state": {
    "desired": {
      "ventilador": "on",
      "bomba": "off",
      "luces": "on",
      "lastUpdated": 1234567890
    }
  }
}
```

**Payload - Reported State**:
```json
{
  "state": {
    "reported": {
      "ventilador": "on",
      "bomba": "off",
      "luces": "on",
      "temperatura": 25.5,
      "humedad": 65.2,
      "lastUpdated": 1234567890
    }
  }
}
```

---

## Umbrales de Alertas

### Temperatura
- **Mínimo**: 15°C (warning)
- **Máximo**: 35°C (warning)
- **Crítico**: 38°C (critical, auto-activa ventilador)

### Humedad Ambiente
- **Mínimo**: 40% (info)
- **Máximo**: 80% (warning)

### Humedad del Suelo
- **Mínimo**: 30% (warning)
- **Crítico**: 20% (critical, auto-activa bomba)
- **Máximo**: 80% (info)

### Luminosidad
- **Mínimo**: 20% (info)

---

## Códigos de Error

### Errores MQTT

| Código | Descripción | Solución |
|--------|-------------|----------|
| -4 | Timeout de conexión | Verificar conectividad de red |
| -3 | Conexión perdida | Verificar estabilidad WiFi |
| -2 | Fallo de conexión | Verificar endpoint IoT |
| -1 | Desconectado | Normal, reconexión automática |
| 0 | Conectado | OK |
| 1 | Protocolo incorrecto | Verificar versión MQTT |
| 2 | ID rechazado | Verificar Thing Name |
| 3 | Servidor no disponible | Verificar región AWS |
| 4 | Credenciales incorrectas | Verificar certificados |
| 5 | No autorizado | Verificar política IoT |

---

## Ejemplos de Uso

### Publicar Datos de Sensores (ESP32)

```cpp
String jsonData = sensorDataToJson(data);
publishSensorData(TOPIC_TEMPERATURA, jsonData);
```

### Controlar Actuador (Dashboard/Lambda)

**Desde AWS IoT Test Client**:
```bash
Topic: invernadero/actuadores/ventilador
Payload: {"state": "on"}
```

**Desde AWS CLI**:
```bash
aws iot-data publish \
  --topic "invernadero/actuadores/ventilador" \
  --payload '{"state":"on"}' \
  --cli-binary-format raw-in-base64-out
```

**Desde Lambda**:
```javascript
const params = {
  topic: 'invernadero/actuadores/ventilador',
  payload: JSON.stringify({ state: 'on' }),
  qos: 1
};
await iot.publish(params).promise();
```

### Suscribirse a Todos los Topics (Testing)

**AWS IoT Test Client**:
```
Topic filter: invernadero/#
```

**MQTT Client (mosquitto_sub)**:
```bash
mosquitto_sub -h xxxxxx-ats.iot.us-east-1.amazonaws.com \
  -p 8883 \
  --cafile AmazonRootCA1.pem \
  --cert device-certificate.pem.crt \
  --key private-key.pem.key \
  -t 'invernadero/#' \
  -v
```

---

## Buenas Prácticas

### Publicación
- ✅ Usar QoS 1 para garantizar entrega
- ✅ Incluir timestamp en todos los mensajes
- ✅ Validar JSON antes de publicar
- ✅ Implementar reintentos con backoff exponencial
- ❌ No publicar más de 100 mensajes/segundo por conexión

### Suscripción
- ✅ Usar wildcards (`#`, `+`) con precaución
- ✅ Implementar manejo de errores en callbacks
- ✅ Validar payload antes de procesar
- ❌ No suscribirse a topics innecesarios

### Seguridad
- ✅ Siempre usar TLS 1.2
- ✅ Rotar certificados periódicamente
- ✅ Usar políticas IoT restrictivas
- ❌ Nunca incluir credenciales en payloads
- ❌ No deshabilitar validación de certificados

---

## Límites AWS IoT Core

| Recurso | Límite |
|---------|--------|
| Mensajes publicados/segundo | 100 por conexión |
| Tamaño máximo de mensaje | 128 KB |
| Conexiones simultáneas | 500,000 por cuenta |
| Suscripciones por conexión | 50 |
| Topics en wildcard | 8 niveles |
| Longitud de topic | 256 bytes |

---

## Monitoreo y Debugging

### CloudWatch Metrics
- `PublishIn.Success`: Mensajes publicados exitosamente
- `PublishIn.Failure`: Mensajes rechazados
- `Connect.Success`: Conexiones exitosas
- `Connect.AuthError`: Errores de autenticación

### CloudWatch Logs
```bash
# Ver logs de reglas IoT
aws logs tail /aws/iot/rules/invernadero_process_sensor_data_prod --follow

# Ver logs de Lambda
aws logs tail /aws/lambda/invernadero-iot-process-sensor-data-prod --follow
```

### AWS IoT Test Client
1. AWS Console → IoT Core → Test
2. Subscribe to topic: `invernadero/#`
3. Observar mensajes en tiempo real

---

## Versionado de API

**Versión actual**: 1.0.0

**Changelog**:
- `1.0.0` (2024-01-01): Versión inicial

**Compatibilidad**:
- Firmware ESP32: v1.0.0+
- Lambda Functions: v1.0.0+

---

## Soporte

Para reportar problemas con la API:
- Abrir issue en GitHub
- Incluir logs de CloudWatch
- Especificar versión de firmware
- Adjuntar ejemplos de payloads
