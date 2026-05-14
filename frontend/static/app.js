/**
 * Sentinel Ride Dashboard — Frontend Logic
 * Real-time monitoring and control for smart helmet safety system
 */

const API_BASE = 'http://localhost:5001/api';

// Application State
const appState = {
    armed: false,
    esp32Online: false,
    crashDetected: false,
    drowsinessDetected: false,
    lastHeartbeat: null
};

/**
 * Initialize Dashboard
 */
function initDashboard() {
    console.log('🛡️ Sentinel Ride Dashboard Initializing...');
    
    // Attach event listeners
    document.getElementById('armBtn').addEventListener('click', armSystem);
    document.getElementById('disarmBtn').addEventListener('click', disarmSystem);
    
    // Start polling for status updates
    pollSystemStatus();
    setInterval(pollSystemStatus, 2000); // Poll every 2 seconds
    
    log('System initialized', 'success');
}

/**
 * Poll System Status from Server
 */
async function pollSystemStatus() {
    try {
        const response = await fetch(`${API_BASE}/status`);
        const data = await response.json();
        
        appState.armed = data.armed;
        appState.esp32Online = data.esp32_online;
        appState.crashDetected = data.crash_detected;
        appState.drowsinessDetected = data.drowsiness_detected;
        appState.lastHeartbeat = data.last_heartbeat;
        
        updateDashboard();
    } catch (error) {
        console.error('❌ Status poll failed:', error);
        log('Status poll failed', 'error');
    }
}

/**
 * Update Dashboard UI
 */
function updateDashboard() {
    // System Status
    const systemStatus = document.querySelector('#systemStatus .status-value');
    if (appState.armed) {
        systemStatus.textContent = 'ARMED';
        systemStatus.className = 'status-value armed';
    } else {
        systemStatus.textContent = 'DISARMED';
        systemStatus.className = 'status-value disarmed';
    }
    
    // Device Status
    const deviceStatus = document.querySelector('#deviceStatus .status-value');
    if (appState.esp32Online) {
        deviceStatus.textContent = 'ONLINE';
        deviceStatus.className = 'status-value normal';
    } else {
        deviceStatus.textContent = 'OFFLINE';
        deviceStatus.className = 'status-value offline';
    }
    
    // Crash Status
    const crashStatus = document.querySelector('#crashStatus .status-value');
    if (appState.crashDetected) {
        crashStatus.textContent = '🚨 CRASH!';
        crashStatus.className = 'status-value error pulse';
    } else {
        crashStatus.textContent = 'NO CRASH';
        crashStatus.className = 'status-value normal';
    }
    
    // Drowsiness Status
    const drowsinessStatus = document.querySelector('#drowsinessStatus .status-value');
    if (appState.drowsinessDetected) {
        drowsinessStatus.textContent = '⚠️ ALERT!';
        drowsinessStatus.className = 'status-value warning pulse';
    } else {
        drowsinessStatus.textContent = 'CLEAR';
        drowsinessStatus.className = 'status-value normal';
    }
    
    // Last Heartbeat
    const heartbeatEl = document.getElementById('lastHeartbeat');
    if (appState.lastHeartbeat) {
        const lastTime = new Date(appState.lastHeartbeat);
        heartbeatEl.textContent = `Last update: ${lastTime.toLocaleTimeString()}`;
    } else {
        heartbeatEl.textContent = 'Last update: Never';
    }
}

/**
 * ARM System
 */
async function armSystem() {
    try {
        const response = await fetch(`${API_BASE}/arm`, { method: 'POST' });
        const data = await response.json();
        log('✅ System ARMED', 'success');
        appState.armed = true;
        updateDashboard();
    } catch (error) {
        console.error('❌ ARM failed:', error);
        log('Failed to arm system', 'error');
    }
}

/**
 * DISARM System
 */
async function disarmSystem() {
    try {
        const response = await fetch(`${API_BASE}/disarm`, { method: 'POST' });
        const data = await response.json();
        log('✅ System DISARMED', 'success');
        appState.armed = false;
        appState.crashDetected = false;
        appState.drowsinessDetected = false;
        updateDashboard();
    } catch (error) {
        console.error('❌ DISARM failed:', error);
        log('Failed to disarm system', 'error');
    }
}

/**
 * Log Messages to Dashboard
 */
function log(message, type = 'info') {
    const logContainer = document.getElementById('activityLog');
    const timestamp = new Date().toLocaleTimeString();
    
    const entry = document.createElement('p');
    entry.className = `log-entry ${type}`;
    entry.textContent = `[${timestamp}] ${message}`;
    
    logContainer.insertBefore(entry, logContainer.firstChild);
    
    // Keep only last 50 entries
    while (logContainer.children.length > 50) {
        logContainer.removeChild(logContainer.lastChild);
    }
}

/**
 * Trigger Emergency Alert via EmailJS
 */
async function sendEmergencyAlert(alertType, data) {
    try {
        console.log(`🚨 Sending emergency alert: ${alertType}`, data);
        
        // TODO: Implement EmailJS integration
        // emailjs.send(SERVICE_ID, TEMPLATE_ID, {
        //     alert_type: alertType,
        //     timestamp: new Date().toISOString(),
        //     device_status: JSON.stringify(appState),
        //     user_email: 'rider@example.com'
        // });
        
        log(`🚨 Emergency alert sent: ${alertType}`, 'error');
    } catch (error) {
        console.error('❌ Alert send failed:', error);
        log('Failed to send emergency alert', 'error');
    }
}

/**
 * WebSocket Listener (optional real-time updates)
 */
function initWebSocket() {
    // TODO: Implement WebSocket for real-time crash/drowsiness alerts
    // const ws = new WebSocket('ws://localhost:5000/ws');
    // ws.onmessage = (event) => {
    //     const data = JSON.parse(event.data);
    //     if (data.type === 'crash') {
    //         sendEmergencyAlert('CRASH', data);
    //     }
    // };
}

// Initialize when page loads
document.addEventListener('DOMContentLoaded', initDashboard);
