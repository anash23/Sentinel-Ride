"""
Sentinel Ride — Smart Helmet Safety System
Flask backend server for ESP32 communication and web dashboard
"""
from flask import Flask, render_template, request, jsonify
from datetime import datetime, timedelta
import json
import os
from pathlib import Path
from dotenv import load_dotenv
import requests

load_dotenv()

# Get the absolute paths
BASE_DIR = Path(__file__).parent.parent
FRONTEND_DIR = BASE_DIR / 'frontend'
STATIC_DIR = FRONTEND_DIR / 'static'

# Create Flask app with absolute paths
app = Flask(
    __name__,
    template_folder=str(FRONTEND_DIR),
    static_folder=str(STATIC_DIR),
    static_url_path='/'
)

# System state
system_state = {
    'armed': False,
    'esp32_online': False,
    'crash_detected': False,
    'drowsiness_detected': False,
    'last_heartbeat': None,
    'device_id': 'ESP32_001',
    'offline_alert_sent': False
}

# Configuration
HEARTBEAT_TIMEOUT = 25  # seconds
DROWSINESS_THRESHOLD = 3  # seconds
EMAILJS_SERVICE_ID = os.getenv('EMAILJS_SERVICE_ID', '')
EMAILJS_TEMPLATE_ID = os.getenv('EMAILJS_TEMPLATE_ID', '')
EMAILJS_PUBLIC_KEY = os.getenv('EMAILJS_PUBLIC_KEY', os.getenv('EMAILJS_USER_ID', ''))
EMAILJS_TO_EMAIL = os.getenv('EMAILJS_TO_EMAIL', 'avinashreddybanuri@gmail.com')
EMAILJS_API_URL = 'https://api.emailjs.com/api/v1.0/email/send'


def send_email_alert(alert_type, subject, message, extra_data=None):
    """Send an emergency email through EmailJS."""
    if not EMAILJS_SERVICE_ID or not EMAILJS_TEMPLATE_ID or not EMAILJS_PUBLIC_KEY or not EMAILJS_TO_EMAIL:
        print('[EMAIL] Skipping alert because EmailJS is not configured')
        return False

    payload = {
        'service_id': EMAILJS_SERVICE_ID,
        'template_id': EMAILJS_TEMPLATE_ID,
        'user_id': EMAILJS_PUBLIC_KEY,
        'template_params': {
            'alert_type': alert_type,
            'subject': subject,
            'message': message,
            'device_id': system_state['device_id'],
            'armed': str(system_state['armed']),
            'esp32_online': str(system_state['esp32_online']),
            'last_heartbeat': str(system_state['last_heartbeat']),
            'to_email': EMAILJS_TO_EMAIL,
            'timestamp': datetime.now().isoformat()
        }
    }

    if extra_data:
        payload['template_params']['details'] = json.dumps(extra_data)

    try:
        response = requests.post(EMAILJS_API_URL, json=payload, timeout=10)
        response.raise_for_status()
        print(f'[EMAIL] Alert sent: {alert_type}')
        return True
    except Exception as exc:
        print(f'[EMAIL] Failed to send alert: {exc}')
        return False


@app.route('/')
def dashboard():
    """Serve the web dashboard."""
    return render_template('index.html')


@app.route('/api/status', methods=['GET'])
def get_status():
    """Get current system status."""
    return jsonify(system_state)


@app.route('/api/arm', methods=['POST'])
def arm_system():
    """ARM the monitoring system."""
    system_state['armed'] = True
    system_state['offline_alert_sent'] = False
    print("[ARM] System armed for monitoring")
    return jsonify({'status': 'armed', 'timestamp': datetime.now().isoformat()})


@app.route('/api/disarm', methods=['POST'])
def disarm_system():
    """DISARM the monitoring system."""
    system_state['armed'] = False
    system_state['crash_detected'] = False
    system_state['drowsiness_detected'] = False
    system_state['offline_alert_sent'] = False
    print("[DISARM] System disarmed")
    return jsonify({'status': 'disarmed', 'timestamp': datetime.now().isoformat()})


@app.route('/api/heartbeat', methods=['POST'])
def receive_heartbeat():
    """Receive heartbeat/status update from ESP32."""
    try:
        data = request.get_json()
        system_state['esp32_online'] = True
        system_state['last_heartbeat'] = datetime.now().isoformat()
        system_state['offline_alert_sent'] = False
        print(f"[HEARTBEAT] {data}")
        return jsonify({'received': True}), 200
    except Exception as e:
        print(f"[ERROR] Heartbeat processing failed: {e}")
        return jsonify({'error': str(e)}), 400


@app.route('/api/crash', methods=['POST'])
def receive_crash():
    """Receive crash detection alert from ESP32."""
    try:
        data = request.get_json()
        system_state['crash_detected'] = True
        print(f"[CRASH] Crash detected! Data: {data}")
        
        if system_state['armed']:
            # Send the requested emergency message with coordinates
            send_email_alert(
                alert_type='crash',
                subject='Sentinel Ride: Crash detected',
                message='Alert!!!! the rider might have crashed at 17.5384240°N 78.3850000°E please respond!!!!!',
                extra_data=data
            )
        
        return jsonify({'crash_received': True}), 200
    except Exception as e:
        print(f"[ERROR] Crash processing failed: {e}")
        return jsonify({'error': str(e)}), 400


@app.route('/api/drowsiness', methods=['POST'])
def receive_drowsiness():
    """Receive drowsiness detection from ESP32."""
    try:
        data = request.get_json()
        system_state['drowsiness_detected'] = True
        print(f"[DROWSINESS] Drowsiness detected! Data: {data}")
        return jsonify({'drowsiness_received': True}), 200
    except Exception as e:
        print(f"[ERROR] Drowsiness processing failed: {e}")
        return jsonify({'error': str(e)}), 400


@app.route('/api/offline-check', methods=['GET'])
def check_offline():
    """Check if device has gone offline (no heartbeat for HEARTBEAT_TIMEOUT)."""
    if system_state['last_heartbeat'] is None:
        return jsonify({'offline': True, 'reason': 'no_heartbeat'})
    
    last_beat = datetime.fromisoformat(system_state['last_heartbeat'])
    time_since = (datetime.now() - last_beat).total_seconds()
    
    is_offline = time_since > HEARTBEAT_TIMEOUT and system_state['armed']

    if is_offline and not system_state['offline_alert_sent']:
        system_state['offline_alert_sent'] = True
        send_email_alert(
            alert_type='offline',
            subject='Sentinel Ride: Device offline',
            message='No heartbeat received from the ESP32 helmet safety system. The device may be offline or the rider may have crashed.',
            extra_data={
                'seconds_since_heartbeat': time_since,
                'threshold': HEARTBEAT_TIMEOUT
            }
        )
    
    return jsonify({
        'offline': is_offline,
        'seconds_since_heartbeat': time_since,
        'threshold': HEARTBEAT_TIMEOUT
    })


@app.errorhandler(404)
def not_found(error):
    return jsonify({'error': 'Not found'}), 404


if __name__ == '__main__':
    print("Starting Sentinel Ride Flask Server...")
    print("Dashboard: http://localhost:5001")
    app.run(debug=True, host='0.0.0.0', port=5001)
