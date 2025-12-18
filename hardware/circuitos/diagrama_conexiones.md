# Diagrama de Conexiones - ESP32 Invernadero IoT

## Diagrama Visual

![Diagrama de Conexiones ESP32](file:///C:/Users/ezequ/.gemini/antigravity/brain/b046f230-eeb1-4ebf-8b9c-63ef6fa2860a/esp32_wiring_diagram_1766030181080.png)

## Tabla de Conexiones

### Sensores

| Componente | Pin Sensor | Pin ESP32 | Notas |
|------------|------------|-----------|-------|
| **DHT22** | | | |
| VCC | VCC | 3.3V | Alimentación |
| GND | GND | GND | Tierra |
| DATA | OUT | GPIO4 | Señal digital |
| **Sensor Humedad Suelo** | | | |
| VCC | VCC | 3.3V | Alimentación |
| GND | GND | GND | Tierra |
| AOUT | AOUT | GPIO34 (ADC1_CH6) | Señal analógica |
| **LDR (Fotoresistencia)** | | | |
| Terminal 1 | - | 3.3V | Con resistencia 10kΩ |
| Terminal 2 | - | GPIO35 (ADC1_CH7) + R 10kΩ a GND | Divisor de voltaje |

### Actuadores (Relays)

| Actuador | Pin Relay | Pin ESP32 | Voltaje Actuador |
|----------|-----------|-----------|------------------|
| **Ventilador** | | | |
| VCC | VCC | 5V | - |
| GND | GND | GND | - |
| IN | IN | GPIO25 | - |
| COM/NO | - | 12V Supply | 12V |
| **Bomba de Riego** | | | |
| VCC | VCC | 5V | - |
| GND | GND | GND | - |
| IN | IN | GPIO26 | - |
| COM/NO | - | 12V Supply | 12V |
| **Luces LED** | | | |
| VCC | VCC | 5V | - |
| GND | GND | GND | - |
| IN | IN | GPIO27 | - |
| COM/NO | - | 12V Supply | 12V |

### Alimentación

| Componente | Voltaje | Corriente | Conexión |
|------------|---------|-----------|----------|
| ESP32 + Relays | 5V | 2A | Fuente 5V → VIN ESP32 |
| Actuadores (Fan, Pump, Lights) | 12V | 3A | Fuente 12V → Relays COM |

## Notas Importantes

### Sensores Analógicos
- **GPIO34 y GPIO35** son pines ADC1 (solo entrada)
- No usar pines ADC2 cuando WiFi está activo
- Rango de lectura: 0-3.3V (0-4095 digital)

### Relays
- Los relays son **activos en LOW** (LOW = encendido, HIGH = apagado)
- Usar módulos relay con optoacoplador para protección
- VCC del relay a 5V, no a 3.3V

### Divisor de Voltaje LDR
```
3.3V ----[LDR]----+----[R 10kΩ]---- GND
                  |
                GPIO35
```

### Calibración
- **Sensor de Humedad Suelo**: Medir en aire (seco) y en agua (húmedo)
- **LDR**: Medir en oscuridad total y con luz brillante
- Ajustar valores en `sensors.cpp`

## Esquema de Pines ESP32

```
                    ESP32 DevKit v1
                   ┌─────────────┐
                   │             │
            3.3V ──┤ 3V3     VIN ├── 5V (Fuente)
             GND ──┤ GND     GND ├── GND
                   │             │
   DHT22 DATA ────┤ GPIO4   GPIO2├── LED Status
                   │             │
Soil Moisture ────┤ GPIO34  GPIO25├── Relay Fan
      LDR ────────┤ GPIO35  GPIO26├── Relay Pump
                   │         GPIO27├── Relay Lights
                   │             │
                   └─────────────┘
```

## Lista de Materiales (BOM)

| Cantidad | Componente | Especificación |
|----------|------------|----------------|
| 1 | ESP32 DevKit v1 | 30 pines |
| 1 | Sensor DHT22 | Temperatura y humedad |
| 1 | Sensor Humedad Suelo | Capacitivo o resistivo |
| 1 | LDR (Fotoresistencia) | 5mm, 5-10kΩ |
| 1 | Resistencia | 10kΩ, 1/4W |
| 3 | Módulo Relay | 5V, 1 canal, con optoacoplador |
| 1 | Ventilador | 12V DC |
| 1 | Bomba de agua | 12V DC, sumergible |
| 1 | Tira LED | 12V DC |
| 1 | Fuente 5V | 2A mínimo |
| 1 | Fuente 12V | 3A mínimo |
| - | Cables jumper | Macho-macho, macho-hembra |
| 1 | Protoboard | 830 puntos (opcional) |

## Seguridad

⚠️ **Advertencias**:
- No conectar actuadores de 12V directamente al ESP32
- Usar fuentes de alimentación separadas para ESP32 y actuadores
- Verificar polaridad antes de conectar
- No exceder 3.3V en pines GPIO
- Usar relays con optoacoplador para aislamiento
- Proteger circuito de humedad en invernadero

## Pruebas

### 1. Probar Sensores
```cpp
// Abrir monitor serial (115200 baud)
// Verificar lecturas cada 30 segundos
```

### 2. Probar Relays
```cpp
// Enviar comando MQTT
// Verificar LED del relay se enciende
// Verificar actuador funciona
```

### 3. Verificar Conexión WiFi
```cpp
// Verificar IP asignada en monitor serial
// Ping al ESP32 desde PC
```

## Troubleshooting

| Problema | Posible Causa | Solución |
|----------|---------------|----------|
| DHT22 retorna NaN | Conexión suelta | Verificar cables, agregar pull-up 10kΩ |
| Sensor suelo siempre 0% | Sensor en aire | Calibrar valores dry/wet |
| LDR no varía | Divisor de voltaje mal | Verificar resistencia 10kΩ |
| Relay no activa | Voltaje incorrecto | Usar 5V para VCC relay |
| ESP32 se reinicia | Sobrecarga corriente | Usar fuentes separadas |
