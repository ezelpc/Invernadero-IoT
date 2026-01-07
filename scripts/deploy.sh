#!/bin/bash

# ============================================
# Script de Despliegue - Invernadero IoT
# ============================================

set -e  # Exit on error

echo "============================================"
echo "Despliegue Invernadero IoT"
echo "============================================"
echo ""

# Variables
PROJECT_NAME="invernadero-iot"
REGION="${AWS_REGION:-us-east-1}"
ENVIRONMENT="${ENVIRONMENT:-prod}"

# Colores
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
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
    exit 1
fi

# Verificar credenciales
if ! aws sts get-caller-identity &> /dev/null; then
    print_error "Credenciales AWS no configuradas"
    exit 1
fi

print_success "Credenciales AWS verificadas"

echo ""
echo "============================================"
echo "1. Empaquetando Funciones Lambda"
echo "============================================"

# Process Sensor Data
print_info "Empaquetando process_sensor_data..."
cd cloud/lambda/process_sensor_data
npm install --production
zip -r function.zip . -x "*.git*" -x "node_modules/.cache/*"
print_success "process_sensor_data empaquetado"

# Control Actuators
cd ../control_actuators
print_info "Empaquetando control_actuators..."
npm install --production
zip -r function.zip . -x "*.git*" -x "node_modules/.cache/*"
print_success "control_actuators empaquetado"

cd ../../..

echo ""
echo "============================================"
echo "2. Desplegando Funciones Lambda"
echo "============================================"

# Deploy Process Sensor Data
print_info "Desplegando process_sensor_data..."
aws lambda update-function-code \
    --function-name "${PROJECT_NAME}-process-sensor-data-${ENVIRONMENT}" \
    --zip-file fileb://cloud/lambda/process_sensor_data/function.zip \
    --region "$REGION" \
    > /dev/null

print_success "process_sensor_data desplegado"

# Deploy Control Actuators
print_info "Desplegando control_actuators..."
aws lambda update-function-code \
    --function-name "${PROJECT_NAME}-control-actuators-${ENVIRONMENT}" \
    --zip-file fileb://cloud/lambda/control_actuators/function.zip \
    --region "$REGION" \
    > /dev/null

print_success "control_actuators desplegado"

echo ""
echo "============================================"
echo "3. Actualizando Stack CloudFormation"
echo "============================================"

print_info "Validando template..."
aws cloudformation validate-template \
    --template-body file://cloud/cloudformation/infrastructure.yml \
    > /dev/null

print_success "Template válido"

print_info "Desplegando stack..."
aws cloudformation deploy \
    --template-file cloud/cloudformation/infrastructure.yml \
    --stack-name "${PROJECT_NAME}-stack-${ENVIRONMENT}" \
    --parameter-overrides \
        ProjectName="$PROJECT_NAME" \
        Environment="$ENVIRONMENT" \
    --capabilities CAPABILITY_NAMED_IAM \
    --region "$REGION"

print_success "Stack CloudFormation actualizado"

echo ""
echo "============================================"
echo "4. Verificando Despliegue"
echo "============================================"

# Verificar funciones Lambda
print_info "Verificando funciones Lambda..."

PROCESS_STATUS=$(aws lambda get-function \
    --function-name "${PROJECT_NAME}-process-sensor-data-${ENVIRONMENT}" \
    --region "$REGION" \
    --query 'Configuration.LastUpdateStatus' \
    --output text)

CONTROL_STATUS=$(aws lambda get-function \
    --function-name "${PROJECT_NAME}-control-actuators-${ENVIRONMENT}" \
    --region "$REGION" \
    --query 'Configuration.LastUpdateStatus' \
    --output text)

if [ "$PROCESS_STATUS" == "Successful" ]; then
    print_success "process_sensor_data: OK"
else
    print_warning "process_sensor_data: $PROCESS_STATUS"
fi

if [ "$CONTROL_STATUS" == "Successful" ]; then
    print_success "control_actuators: OK"
else
    print_warning "control_actuators: $CONTROL_STATUS"
fi

# Verificar stack
print_info "Verificando stack CloudFormation..."
STACK_STATUS=$(aws cloudformation describe-stacks \
    --stack-name "${PROJECT_NAME}-stack-${ENVIRONMENT}" \
    --region "$REGION" \
    --query 'Stacks[0].StackStatus' \
    --output text)

if [[ "$STACK_STATUS" == *"COMPLETE"* ]]; then
    print_success "Stack CloudFormation: $STACK_STATUS"
else
    print_warning "Stack CloudFormation: $STACK_STATUS"
fi

echo ""
echo "============================================"
echo "5. Información del Despliegue"
echo "============================================"

aws cloudformation describe-stacks \
    --stack-name "${PROJECT_NAME}-stack-${ENVIRONMENT}" \
    --region "$REGION" \
    --query 'Stacks[0].Outputs' \
    --output table

echo ""
echo "============================================"
echo "✅ DESPLIEGUE COMPLETADO"
echo "============================================"
echo ""
echo "Ambiente: $ENVIRONMENT"
echo "Región: $REGION"
echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')"
echo ""
echo "🔍 Monitorear logs:"
echo "   aws logs tail /aws/lambda/${PROJECT_NAME}-process-sensor-data-${ENVIRONMENT} --follow"
echo ""
echo "📊 Ver métricas en CloudWatch:"
echo "   https://console.aws.amazon.com/cloudwatch/home?region=${REGION}"
echo ""
