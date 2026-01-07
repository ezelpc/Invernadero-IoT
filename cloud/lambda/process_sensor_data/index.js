const AWS = require('aws-sdk');
const { v4: uuidv4 } = require('uuid');

const dynamodb = new AWS.DynamoDB.DocumentClient();
const iot = new AWS.IotData({ endpoint: process.env.IOT_ENDPOINT });

const TABLE_NAME = process.env.DYNAMODB_TABLE || 'InvernaderoSensorData';
const TOPIC_ALERTAS = 'invernadero/alertas';

// Umbrales de alertas (pueden venir de DynamoDB o variables de entorno)
const THRESHOLDS = {
  temperatura: { min: 15, max: 35, critical: 38 },
  humedad: { min: 40, max: 80 },
  humedadSuelo: { min: 30, max: 80, critical: 20 },
  luminosidad: { min: 20, max: 100 }
};

/**
 * Handler principal de Lambda
 * Procesa datos de sensores desde AWS IoT Core
 */
exports.handler = async (event) => {
  console.log('Evento recibido:', JSON.stringify(event, null, 2));
  
  try {
    // Validar datos de entrada
    if (!event.thing || !event.timestamp) {
      throw new Error('Datos incompletos: se requiere thing y timestamp');
    }
    
    const sensorData = {
      id: uuidv4(),
      thingName: event.thing,
      timestamp: event.timestamp,
      receivedAt: Date.now(),
      temperatura: parseFloat(event.temperatura) || null,
      humedad: parseFloat(event.humedad) || null,
      humedadSuelo: parseFloat(event.humedadSuelo) || null,
      luminosidad: parseFloat(event.luminosidad) || null
    };
    
    // Validar que al menos un sensor tenga datos
    if (!sensorData.temperatura && !sensorData.humedad && 
        !sensorData.humedadSuelo && !sensorData.luminosidad) {
      throw new Error('No se recibieron datos de sensores válidos');
    }
    
    console.log('Datos procesados:', sensorData);
    
    // Guardar en DynamoDB
    await saveToDynamoDB(sensorData);
    
    // Evaluar umbrales y generar alertas
    const alerts = evaluateThresholds(sensorData);
    
    if (alerts.length > 0) {
      console.log('Alertas generadas:', alerts);
      await publishAlerts(sensorData.thingName, alerts);
    }
    
    return {
      statusCode: 200,
      body: JSON.stringify({
        message: 'Datos procesados exitosamente',
        id: sensorData.id,
        alertsGenerated: alerts.length
      })
    };
    
  } catch (error) {
    console.error('Error procesando datos:', error);
    
    return {
      statusCode: 500,
      body: JSON.stringify({
        message: 'Error procesando datos de sensores',
        error: error.message
      })
    };
  }
};

/**
 * Guarda datos en DynamoDB
 */
async function saveToDynamoDB(data) {
  const params = {
    TableName: TABLE_NAME,
    Item: data
  };
  
  try {
    await dynamodb.put(params).promise();
    console.log('Datos guardados en DynamoDB:', data.id);
  } catch (error) {
    console.error('Error guardando en DynamoDB:', error);
    throw error;
  }
}

/**
 * Evalúa umbrales y genera alertas
 */
function evaluateThresholds(data) {
  const alerts = [];
  
  // Evaluar temperatura
  if (data.temperatura !== null) {
    if (data.temperatura < THRESHOLDS.temperatura.min) {
      alerts.push({
        type: 'temperatura',
        severity: 'warning',
        message: `Temperatura muy baja: ${data.temperatura}°C`,
        value: data.temperatura,
        threshold: THRESHOLDS.temperatura.min
      });
    } else if (data.temperatura > THRESHOLDS.temperatura.critical) {
      alerts.push({
        type: 'temperatura',
        severity: 'critical',
        message: `Temperatura crítica: ${data.temperatura}°C`,
        value: data.temperatura,
        threshold: THRESHOLDS.temperatura.critical,
        action: 'Activar ventilador inmediatamente'
      });
    } else if (data.temperatura > THRESHOLDS.temperatura.max) {
      alerts.push({
        type: 'temperatura',
        severity: 'warning',
        message: `Temperatura alta: ${data.temperatura}°C`,
        value: data.temperatura,
        threshold: THRESHOLDS.temperatura.max
      });
    }
  }
  
  // Evaluar humedad del suelo
  if (data.humedadSuelo !== null) {
    if (data.humedadSuelo < THRESHOLDS.humedadSuelo.critical) {
      alerts.push({
        type: 'humedad_suelo',
        severity: 'critical',
        message: `Suelo muy seco: ${data.humedadSuelo}%`,
        value: data.humedadSuelo,
        threshold: THRESHOLDS.humedadSuelo.critical,
        action: 'Activar riego inmediatamente'
      });
    } else if (data.humedadSuelo < THRESHOLDS.humedadSuelo.min) {
      alerts.push({
        type: 'humedad_suelo',
        severity: 'warning',
        message: `Suelo seco: ${data.humedadSuelo}%`,
        value: data.humedadSuelo,
        threshold: THRESHOLDS.humedadSuelo.min
      });
    } else if (data.humedadSuelo > THRESHOLDS.humedadSuelo.max) {
      alerts.push({
        type: 'humedad_suelo',
        severity: 'info',
        message: `Suelo muy húmedo: ${data.humedadSuelo}%`,
        value: data.humedadSuelo,
        threshold: THRESHOLDS.humedadSuelo.max
      });
    }
  }
  
  // Evaluar humedad ambiente
  if (data.humedad !== null) {
    if (data.humedad < THRESHOLDS.humedad.min) {
      alerts.push({
        type: 'humedad',
        severity: 'info',
        message: `Humedad ambiente baja: ${data.humedad}%`,
        value: data.humedad,
        threshold: THRESHOLDS.humedad.min
      });
    } else if (data.humedad > THRESHOLDS.humedad.max) {
      alerts.push({
        type: 'humedad',
        severity: 'warning',
        message: `Humedad ambiente alta: ${data.humedad}%`,
        value: data.humedad,
        threshold: THRESHOLDS.humedad.max
      });
    }
  }
  
  // Evaluar luminosidad
  if (data.luminosidad !== null && data.luminosidad < THRESHOLDS.luminosidad.min) {
    alerts.push({
      type: 'luminosidad',
      severity: 'info',
      message: `Poca luz detectada: ${data.luminosidad}%`,
      value: data.luminosidad,
      threshold: THRESHOLDS.luminosidad.min
    });
  }
  
  return alerts;
}

/**
 * Publica alertas a topic MQTT
 */
async function publishAlerts(thingName, alerts) {
  const payload = {
    thing: thingName,
    timestamp: Date.now(),
    alerts: alerts
  };
  
  const params = {
    topic: TOPIC_ALERTAS,
    payload: JSON.stringify(payload),
    qos: 1
  };
  
  try {
    await iot.publish(params).promise();
    console.log('Alertas publicadas a MQTT');
  } catch (error) {
    console.error('Error publicando alertas:', error);
    // No lanzar error, solo registrar
  }
}
