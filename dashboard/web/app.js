// ============================================
// CONFIGURACIÓN
// ============================================

const CONFIG = {
  // Configurar con tus credenciales AWS
  region: 'us-east-1',
  iotEndpoint: 'xxxxxxxxxxxxxx-ats.iot.us-east-1.amazonaws.com',
  accessKeyId: 'TU_ACCESS_KEY_ID',
  secretAccessKey: 'TU_SECRET_ACCESS_KEY',
  
  // Topics MQTT
  topics: {
    temperatura: 'invernadero/sensores/temperatura',
    humedad: 'invernadero/sensores/humedad',
    luminosidad: 'invernadero/sensores/luminosidad',
    humedadSuelo: 'invernadero/sensores/humedad-suelo',
    alertas: 'invernadero/alertas',
    ventilador: 'invernadero/actuadores/ventilador',
    bomba: 'invernadero/actuadores/bomba',
    luces: 'invernadero/actuadores/luces'
  },
  
  // Configuración de gráficos
  chartUpdateInterval: 30000, // 30 segundos
  maxDataPoints: 48 // 24 horas con lecturas cada 30 min
};

// ============================================
// VARIABLES GLOBALES
// ============================================

let iotData = null;
let isConnected = false;
let tempChart = null;
let humidityChart = null;
let sensorHistory = {
  temperatura: [],
  humedad: [],
  humedadSuelo: [],
  timestamps: []
};

// ============================================
// INICIALIZACIÓN
// ============================================

document.addEventListener('DOMContentLoaded', () => {
  console.log('Inicializando dashboard...');
  
  // Inicializar AWS SDK
  initAWS();
  
  // Inicializar gráficos
  initCharts();
  
  // Configurar event listeners para actuadores
  setupActuatorControls();
  
  // Simular datos para demo (comentar cuando uses AWS real)
  // startDemoMode();
});

// ============================================
// AWS IoT CORE
// ============================================

function initAWS() {
  try {
    // Configurar credenciales AWS
    AWS.config.update({
      region: CONFIG.region,
      credentials: new AWS.Credentials({
        accessKeyId: CONFIG.accessKeyId,
        secretAccessKey: CONFIG.secretAccessKey
      })
    });
    
    // Crear cliente IoT Data
    iotData = new AWS.IotData({
      endpoint: CONFIG.iotEndpoint
    });
    
    updateConnectionStatus(true);
    console.log('AWS IoT configurado correctamente');
    
    // Suscribirse a topics (requiere implementación con WebSockets)
    // Para producción, usar AWS IoT WebSocket o API Gateway
    subscribeToTopics();
    
  } catch (error) {
    console.error('Error configurando AWS:', error);
    updateConnectionStatus(false);
    showAlert('Error de conexión con AWS IoT', 'error');
  }
}

function subscribeToTopics() {
  // NOTA: AWS IoT Data Plane no soporta suscripción directa desde navegador
  // Opciones de implementación:
  // 1. Usar AWS IoT Device SDK para JavaScript con WebSockets
  // 2. Usar API Gateway + WebSocket API
  // 3. Polling periódico a DynamoDB vía API Gateway
  
  // Para este ejemplo, usaremos polling a DynamoDB
  startPolling();
}

function startPolling() {
  // Obtener datos cada 30 segundos
  setInterval(() => {
    fetchLatestSensorData();
  }, CONFIG.chartUpdateInterval);
  
  // Primera carga inmediata
  fetchLatestSensorData();
}

async function fetchLatestSensorData() {
  // NOTA: Implementar endpoint API Gateway que consulte DynamoDB
  // Por ahora, simulamos datos
  console.log('Obteniendo datos de sensores...');
  
  // En producción, hacer fetch a tu API Gateway:
  // const response = await fetch('https://tu-api-gateway.amazonaws.com/prod/sensors/latest');
  // const data = await response.json();
  // updateSensorDisplays(data);
}

function publishActuatorCommand(actuator, state) {
  if (!iotData) {
    console.error('IoT Data no inicializado');
    return;
  }
  
  const topic = CONFIG.topics[actuator];
  const payload = JSON.stringify({
    state: state ? 'on' : 'off',
    timestamp: Date.now(),
    source: 'dashboard'
  });
  
  const params = {
    topic: topic,
    payload: payload,
    qos: 1
  };
  
  iotData.publish(params, (err, data) => {
    if (err) {
      console.error('Error publicando comando:', err);
      showAlert(`Error controlando ${actuator}`, 'error');
    } else {
      console.log(`Comando enviado a ${actuator}:`, state ? 'ON' : 'OFF');
      showAlert(`${actuator} ${state ? 'encendido' : 'apagado'}`, 'success');
    }
  });
}

// ============================================
// ACTUALIZACIÓN DE INTERFAZ
// ============================================

function updateSensorDisplays(data) {
  // Actualizar valores
  updateSensorValue('temperatura', data.temperatura, '°C');
  updateSensorValue('humedad', data.humedad, '%');
  updateSensorValue('humedadSuelo', data.humedadSuelo, '%');
  updateSensorValue('luminosidad', data.luminosidad, '%');
  
  // Actualizar estados
  updateSensorStatus('tempStatus', data.temperatura, 15, 35);
  updateSensorStatus('humStatus', data.humedad, 40, 80);
  updateSensorStatus('soilStatus', data.humedadSuelo, 30, 80);
  updateSensorStatus('luxStatus', data.luminosidad, 20, 100);
  
  // Actualizar historial
  addToHistory(data);
  
  // Actualizar timestamp
  document.getElementById('lastUpdate').textContent = new Date().toLocaleString('es-ES');
}

function updateSensorValue(id, value, unit) {
  const element = document.getElementById(id);
  if (element && value !== null && value !== undefined) {
    element.textContent = value.toFixed(1);
  }
}

function updateSensorStatus(id, value, min, max) {
  const element = document.getElementById(id);
  if (!element || value === null || value === undefined) return;
  
  if (value < min) {
    element.textContent = 'Bajo';
    element.className = 'sensor-status warning';
  } else if (value > max) {
    element.textContent = 'Alto';
    element.className = 'sensor-status critical';
  } else {
    element.textContent = 'Normal';
    element.className = 'sensor-status normal';
  }
}

function updateConnectionStatus(connected) {
  isConnected = connected;
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('connectionStatus');
  
  if (connected) {
    statusDot.className = 'status-dot connected';
    statusText.textContent = 'Conectado';
  } else {
    statusDot.className = 'status-dot disconnected';
    statusText.textContent = 'Desconectado';
  }
}

function showAlert(message, type = 'info') {
  const container = document.getElementById('alertsContainer');
  const alert = document.createElement('div');
  alert.className = `alert alert-${type}`;
  alert.textContent = message;
  
  container.appendChild(alert);
  
  // Auto-remover después de 5 segundos
  setTimeout(() => {
    alert.style.opacity = '0';
    setTimeout(() => alert.remove(), 300);
  }, 5000);
}

// ============================================
// GRÁFICOS
// ============================================

function initCharts() {
  const tempCtx = document.getElementById('tempChart').getContext('2d');
  const humidityCtx = document.getElementById('humidityChart').getContext('2d');
  
  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        labels: {
          color: '#e0e0e0',
          font: { family: 'Inter' }
        }
      }
    },
    scales: {
      x: {
        ticks: { color: '#a0a0a0' },
        grid: { color: 'rgba(255, 255, 255, 0.1)' }
      },
      y: {
        ticks: { color: '#a0a0a0' },
        grid: { color: 'rgba(255, 255, 255, 0.1)' }
      }
    }
  };
  
  tempChart = new Chart(tempCtx, {
    type: 'line',
    data: {
      labels: [],
      datasets: [{
        label: 'Temperatura (°C)',
        data: [],
        borderColor: '#ff6b6b',
        backgroundColor: 'rgba(255, 107, 107, 0.1)',
        tension: 0.4
      }]
    },
    options: chartOptions
  });
  
  humidityChart = new Chart(humidityCtx, {
    type: 'line',
    data: {
      labels: [],
      datasets: [
        {
          label: 'Humedad Ambiente (%)',
          data: [],
          borderColor: '#4ecdc4',
          backgroundColor: 'rgba(78, 205, 196, 0.1)',
          tension: 0.4
        },
        {
          label: 'Humedad Suelo (%)',
          data: [],
          borderColor: '#95e1d3',
          backgroundColor: 'rgba(149, 225, 211, 0.1)',
          tension: 0.4
        }
      ]
    },
    options: chartOptions
  });
}

function addToHistory(data) {
  const timestamp = new Date().toLocaleTimeString('es-ES', { hour: '2-digit', minute: '2-digit' });
  
  sensorHistory.timestamps.push(timestamp);
  sensorHistory.temperatura.push(data.temperatura);
  sensorHistory.humedad.push(data.humedad);
  sensorHistory.humedadSuelo.push(data.humedadSuelo);
  
  // Limitar a maxDataPoints
  if (sensorHistory.timestamps.length > CONFIG.maxDataPoints) {
    sensorHistory.timestamps.shift();
    sensorHistory.temperatura.shift();
    sensorHistory.humedad.shift();
    sensorHistory.humedadSuelo.shift();
  }
  
  updateCharts();
}

function updateCharts() {
  // Actualizar gráfico de temperatura
  tempChart.data.labels = sensorHistory.timestamps;
  tempChart.data.datasets[0].data = sensorHistory.temperatura;
  tempChart.update();
  
  // Actualizar gráfico de humedad
  humidityChart.data.labels = sensorHistory.timestamps;
  humidityChart.data.datasets[0].data = sensorHistory.humedad;
  humidityChart.data.datasets[1].data = sensorHistory.humedadSuelo;
  humidityChart.update();
}

// ============================================
// CONTROL DE ACTUADORES
// ============================================

function setupActuatorControls() {
  document.getElementById('ventiladorSwitch').addEventListener('change', (e) => {
    handleActuatorToggle('ventilador', e.target.checked);
  });
  
  document.getElementById('bombaSwitch').addEventListener('change', (e) => {
    handleActuatorToggle('bomba', e.target.checked);
  });
  
  document.getElementById('lucesSwitch').addEventListener('change', (e) => {
    handleActuatorToggle('luces', e.target.checked);
  });
}

function handleActuatorToggle(actuator, state) {
  console.log(`Toggle ${actuator}: ${state}`);
  
  // Actualizar UI
  const stateElement = document.getElementById(`${actuator}State`);
  stateElement.textContent = state ? 'Encendido' : 'Apagado';
  stateElement.className = `actuator-state ${state ? 'active' : ''}`;
  
  // Publicar comando a AWS IoT
  publishActuatorCommand(actuator, state);
}

// ============================================
// MODO DEMO (para pruebas sin AWS)
// ============================================

function startDemoMode() {
  console.log('Iniciando modo demo...');
  updateConnectionStatus(true);
  
  setInterval(() => {
    const demoData = {
      temperatura: 20 + Math.random() * 10,
      humedad: 50 + Math.random() * 20,
      humedadSuelo: 40 + Math.random() * 30,
      luminosidad: 30 + Math.random() * 50
    };
    
    updateSensorDisplays(demoData);
  }, 3000);
}
