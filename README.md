# 🌱 Invernadero-IoT

**IoT + AWS + DevSecOps reference project for environmental monitoring and automated greenhouse control.**

ESP32 sensors publish telemetry over MQTT/TLS to AWS IoT Core. Cloud functions process the data, persist measurements and publish control commands back to the device.

> **Portfolio focus:** embedded systems, AWS IoT, cloud architecture, secure device communication and CI/CD.

## Architecture

```text
ESP32 + Sensors
      │
      │ MQTT / TLS
      ▼
AWS IoT Core
   │       │
   │       └──────────────► Device commands
   ▼
AWS Lambda
   │
   ├──────────► DynamoDB
   ├──────────► CloudWatch
   └──────────► Alerts / Dashboard

GitHub Actions
      │
      └──────► Firmware + cloud validation
```

## Key capabilities

- 📡 Temperature, humidity, luminosity and soil-moisture telemetry
- 🔐 MQTT over TLS for device-to-cloud communication
- 💧 Automated irrigation control
- 💨 Ventilation and lighting control
- 🚨 Threshold-based alerts
- 📊 Historical telemetry storage
- ☁️ AWS IoT Core, Lambda, DynamoDB, S3 and CloudWatch
- ⚙️ GitHub Actions / PlatformIO automation
- 🏗️ CloudFormation-based infrastructure

## Technology stack

| Layer | Technology |
|---|---|
| Device | ESP32 DevKit v1 |
| Sensors | DHT22, soil-moisture sensor, LDR |
| Messaging | MQTT / AWS IoT Core |
| Compute | AWS Lambda |
| Data | DynamoDB |
| Storage | Amazon S3 |
| Observability | CloudWatch |
| Frontend | HTML, CSS, JavaScript, Chart.js |
| IaC | AWS CloudFormation |
| CI/CD | GitHub Actions + PlatformIO |

## Repository structure

```text
Invernadero-IoT/
├── .github/workflows/     # CI/CD
├── hardware/
│   ├── esp32/             # Firmware
│   └── circuitos/         # Hardware diagrams
├── cloud/
│   ├── lambda/            # Serverless functions
│   ├── iot-policies/      # AWS IoT policies
│   └── cloudformation/    # Infrastructure as Code
├── dashboard/
│   ├── grafana/           # Dashboards
│   └── web/               # Web dashboard
├── docs/                  # Architecture and setup documentation
└── scripts/               # Automation
```

## MQTT topics

```text
invernadero/sensores/temperatura
invernadero/sensores/humedad
invernadero/sensores/luminosidad
invernadero/sensores/humedad-suelo
invernadero/actuadores/ventilador
invernadero/actuadores/bomba
invernadero/actuadores/luces
invernadero/alertas
invernadero/estado
```

## Security controls

The security model should treat the ESP32 as an untrusted edge device:

- TLS-protected MQTT communication
- Per-device certificates and restrictive IoT policies
- Least-privilege Lambda execution roles
- No AWS credentials embedded in firmware
- Secrets kept outside source control
- CloudWatch audit and operational logs
- Infrastructure reviewed as code before deployment

### Example IoT policy principle

Device identities should only publish/subscribe to the MQTT topics they actually require. Avoid wildcard permissions such as unrestricted `iot:*` access.

## Quick start

### Prerequisites

- ESP32
- DHT22
- Soil-moisture sensor
- LDR
- Relay module
- PlatformIO
- AWS CLI
- Authorized AWS account with IoT Core, Lambda and DynamoDB access

### Clone

```bash
git clone https://github.com/ezelpc/Invernadero-IoT.git
cd Invernadero-IoT
```

### AWS configuration

Use an approved local AWS credential mechanism such as an AWS CLI profile or environment managed by your organization. **Never commit access keys to the repository.**

```bash
aws sts get-caller-identity
bash scripts/setup-aws.sh
```

### Firmware

```bash
cd hardware/esp32
pio run
pio run --target upload
```

## ⚠️ Cost and deployment note

AWS IoT Core, Lambda, DynamoDB, S3, CloudWatch and related services can incur charges depending on usage, region and account configuration. Do **not** assume the entire stack is permanently free.

Before deployment:

1. Review current AWS pricing.
2. Configure billing alerts/budgets.
3. Limit telemetry frequency.
4. Remove unused resources after testing.
5. Use a dedicated lab account when possible.

## Documentation

- [Architecture](docs/arquitectura.md)
- [Configuration](docs/guia-configuracion.md)
- [MQTT API](docs/documentacion-api.md)
- [ESP32](hardware/esp32/README.md)

## Contributors

- Castañeda Ramírez Gabriel Eduardo
- Fernández Baños Diana Paola
- Rodríguez Núñez Ángel
- Pérez Carbajal Ezequiel Nahun

## License

MIT — see [LICENSE](LICENSE).
