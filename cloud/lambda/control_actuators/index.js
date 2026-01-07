const AWS = require('aws-sdk');

const iot = new AWS.IotData({ endpoint: process.env.IOT_ENDPOINT });
const dynamodb = new AWS.DynamoDB.DocumentClient();

const ACTIONS_TABLE = process.env.ACTIONS_TABLE || 'InvernaderoActuatorActions';

/**
 * Handler principal de Lambda
 * Controla actuadores mediante comandos MQTT
 */
exports.handler = async (event) => {
  console.log('Evento recibido:', JSON.stringify(event, null, 2));
  
  try {
    // Validar datos de entrada
    const { thing, actuator, state, source } = event;
    
    if (!thing || !actuator || state === undefined) {
      throw new Error('Datos incompletos: se requiere thing, actuator y state');
    }
    
    // Validar actuador
    const validActuators = ['ventilador', 'bomba', 'luces'];
    if (!validActuators.includes(actuator)) {
      throw new Error(`Actuador inválido: ${actuator}. Válidos: ${validActuators.join(', ')}`);
    }
    
    // Validar estado
    const validStates = ['on', 'off', 'ON', 'OFF', true, false, 1, 0];
    if (!validStates.includes(state)) {
      throw new Error(`Estado inválido: ${state}`);
    }
    
    // Normalizar estado a booleano
    const normalizedState = (state === 'on' || state === 'ON' || state === true || state === 1);
    
    console.log(`Controlando ${actuator} del thing ${thing}: ${normalizedState ? 'ON' : 'OFF'}`);
    
    // Publicar comando a topic MQTT
    const topic = `invernadero/actuadores/${actuator}`;
    await publishActuatorCommand(topic, normalizedState);
    
    // Actualizar shadow del dispositivo
    await updateThingShadow(thing, actuator, normalizedState);
    
    // Registrar acción en DynamoDB
    await logAction(thing, actuator, normalizedState, source || 'manual');
    
    return {
      statusCode: 200,
      body: JSON.stringify({
        message: 'Comando enviado exitosamente',
        thing: thing,
        actuator: actuator,
        state: normalizedState ? 'on' : 'off',
        topic: topic
      })
    };
    
  } catch (error) {
    console.error('Error controlando actuador:', error);
    
    return {
      statusCode: 500,
      body: JSON.stringify({
        message: 'Error controlando actuador',
        error: error.message
      })
    };
  }
};

/**
 * Publica comando de actuador a topic MQTT
 */
async function publishActuatorCommand(topic, state) {
  const payload = {
    state: state ? 'on' : 'off',
    timestamp: Date.now()
  };
  
  const params = {
    topic: topic,
    payload: JSON.stringify(payload),
    qos: 1
  };
  
  try {
    await iot.publish(params).promise();
    console.log(`Comando publicado a ${topic}:`, payload);
  } catch (error) {
    console.error('Error publicando comando:', error);
    throw error;
  }
}

/**
 * Actualiza el shadow del dispositivo
 */
async function updateThingShadow(thingName, actuator, state) {
  const shadowPayload = {
    state: {
      desired: {
        [actuator]: state ? 'on' : 'off',
        lastUpdated: Date.now()
      }
    }
  };
  
  const params = {
    thingName: thingName,
    payload: JSON.stringify(shadowPayload)
  };
  
  try {
    await iot.updateThingShadow(params).promise();
    console.log(`Shadow actualizado para ${thingName}:`, shadowPayload);
  } catch (error) {
    console.error('Error actualizando shadow:', error);
    // No lanzar error, solo registrar
  }
}

/**
 * Registra la acción en DynamoDB para auditoría
 */
async function logAction(thing, actuator, state, source) {
  const params = {
    TableName: ACTIONS_TABLE,
    Item: {
      id: `${thing}-${Date.now()}`,
      thingName: thing,
      actuator: actuator,
      state: state ? 'on' : 'off',
      source: source,
      timestamp: Date.now(),
      executedAt: new Date().toISOString()
    }
  };
  
  try {
    await dynamodb.put(params).promise();
    console.log('Acción registrada en DynamoDB');
  } catch (error) {
    console.error('Error registrando acción:', error);
    // No lanzar error, solo registrar
  }
}
