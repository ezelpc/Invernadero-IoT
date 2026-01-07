#!/bin/bash

# ============================================
# Script de Configuración AWS IoT Core
# ============================================

set -e  # Exit on error

echo "============================================"
echo "Configuración AWS IoT Core - Invernadero IoT"
echo "============================================"
echo ""

# Variables
PROJECT_NAME="invernadero-iot"
THING_NAME="invernadero-01"
POLICY_NAME="${PROJECT_NAME}-thing-policy"
REGION="${AWS_REGION:-us-east-1}"
CERTS_DIR="./certs"

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Funciones auxiliares
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Verificar AWS CLI
if ! command -v aws &> /dev/null; then
    print_error "AWS CLI no está instalado"
    echo "Instalar desde: https://aws.amazon.com/cli/"
    exit 1
fi

print_success "AWS CLI encontrado"

# Verificar credenciales AWS
if ! aws sts get-caller-identity &> /dev/null; then
    print_error "Credenciales AWS no configuradas"
    echo "Ejecutar: aws configure"
    exit 1
fi

print_success "Credenciales AWS verificadas"

# Crear directorio para certificados
mkdir -p "$CERTS_DIR"
print_success "Directorio de certificados creado: $CERTS_DIR"

echo ""
echo "============================================"
echo "1. Creando IoT Thing"
echo "============================================"

# Verificar si el Thing ya existe
if aws iot describe-thing --thing-name "$THING_NAME" --region "$REGION" &> /dev/null; then
    print_warning "Thing '$THING_NAME' ya existe"
else
    aws iot create-thing \
        --thing-name "$THING_NAME" \
        --region "$REGION"
    print_success "Thing '$THING_NAME' creado"
fi

echo ""
echo "============================================"
echo "2. Generando Certificados"
echo "============================================"

# Crear certificados y claves
CERT_OUTPUT=$(aws iot create-keys-and-certificate \
    --set-as-active \
    --certificate-pem-outfile "$CERTS_DIR/device-certificate.pem.crt" \
    --public-key-outfile "$CERTS_DIR/public-key.pem.key" \
    --private-key-outfile "$CERTS_DIR/private-key.pem.key" \
    --region "$REGION")

CERTIFICATE_ARN=$(echo "$CERT_OUTPUT" | grep -o '"certificateArn": "[^"]*' | cut -d'"' -f4)
CERTIFICATE_ID=$(echo "$CERT_OUTPUT" | grep -o '"certificateId": "[^"]*' | cut -d'"' -f4)

print_success "Certificados generados"
echo "Certificate ID: $CERTIFICATE_ID"

# Descargar Amazon Root CA
curl -s https://www.amazontrust.com/repository/AmazonRootCA1.pem -o "$CERTS_DIR/AmazonRootCA1.pem"
print_success "Amazon Root CA descargado"

echo ""
echo "============================================"
echo "3. Creando Política IoT"
echo "============================================"

# Verificar si la política ya existe
if aws iot get-policy --policy-name "$POLICY_NAME" --region "$REGION" &> /dev/null; then
    print_warning "Política '$POLICY_NAME' ya existe, actualizando..."
    
    # Crear nueva versión de la política
    aws iot create-policy-version \
        --policy-name "$POLICY_NAME" \
        --policy-document file://cloud/iot-policies/thing-policy.json \
        --set-as-default \
        --region "$REGION"
    print_success "Política actualizada"
else
    aws iot create-policy \
        --policy-name "$POLICY_NAME" \
        --policy-document file://cloud/iot-policies/thing-policy.json \
        --region "$REGION"
    print_success "Política '$POLICY_NAME' creada"
fi

echo ""
echo "============================================"
echo "4. Adjuntando Política y Certificado"
echo "============================================"

# Adjuntar política al certificado
aws iot attach-policy \
    --policy-name "$POLICY_NAME" \
    --target "$CERTIFICATE_ARN" \
    --region "$REGION"
print_success "Política adjuntada al certificado"

# Adjuntar certificado al Thing
aws iot attach-thing-principal \
    --thing-name "$THING_NAME" \
    --principal "$CERTIFICATE_ARN" \
    --region "$REGION"
print_success "Certificado adjuntado al Thing"

echo ""
echo "============================================"
echo "5. Desplegando Infraestructura CloudFormation"
echo "============================================"

aws cloudformation deploy \
    --template-file cloud/cloudformation/infrastructure.yml \
    --stack-name "${PROJECT_NAME}-stack-prod" \
    --parameter-overrides \
        ProjectName="$PROJECT_NAME" \
        Environment=prod \
    --capabilities CAPABILITY_NAMED_IAM \
    --region "$REGION"

print_success "Stack CloudFormation desplegado"

echo ""
echo "============================================"
echo "6. Obteniendo Información del Stack"
echo "============================================"

aws cloudformation describe-stacks \
    --stack-name "${PROJECT_NAME}-stack-prod" \
    --region "$REGION" \
    --query 'Stacks[0].Outputs' \
    --output table

echo ""
echo "============================================"
echo "✅ CONFIGURACIÓN COMPLETADA"
echo "============================================"
echo ""

# Obtener endpoint IoT
IOT_ENDPOINT=$(aws iot describe-endpoint --endpoint-type iot:Data-ATS --region "$REGION" --query 'endpointAddress' --output text)

echo "📋 INFORMACIÓN IMPORTANTE:"
echo ""
echo "Thing Name: $THING_NAME"
echo "IoT Endpoint: $IOT_ENDPOINT"
echo "Región: $REGION"
echo ""
echo "📁 Certificados guardados en: $CERTS_DIR/"
echo "   - AmazonRootCA1.pem"
echo "   - device-certificate.pem.crt"
echo "   - private-key.pem.key"
echo ""
echo "⚠️  IMPORTANTE:"
echo "1. Copiar los certificados a hardware/esp32/src/config.h"
echo "2. Actualizar AWS_IOT_ENDPOINT en config.h con: $IOT_ENDPOINT"
echo "3. Configurar WIFI_SSID y WIFI_PASSWORD"
echo "4. NO subir los certificados a Git (ya están en .gitignore)"
echo ""
echo "🚀 Siguiente paso: Compilar y cargar firmware ESP32"
echo "   cd hardware/esp32"
echo "   pio run --target upload"
echo ""
