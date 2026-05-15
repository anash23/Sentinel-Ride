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

# Load .env from backend folder or root
env_path = Path(__file__).parent / '.env'  # backend/.env
if not env_path.exists():
    env_path = Path(__file__).parent.parent / '.env'  # root/.env
load_dotenv(env_path)

# Debug: report whether EmailJS env vars are present (do not print secrets)
try:
    _svc_set = bool(os.getenv('EMAILJS_SERVICE_ID', ''))
    _tmpl_set = bool(os.getenv('EMAILJS_TEMPLATE_ID', ''))
    _pub_set = bool(os.getenv('EMAILJS_PUBLIC_KEY', os.getenv('EMAILJS_USER_ID', '')))
    _to_set = bool(os.getenv('EMAILJS_TO_EMAIL', ''))
    print(f"[EMAIL CONFIG] service_id_set={_svc_set}, template_id_set={_tmpl_set}, public_key_set={_pub_set}, to_email_set={_to_set}")
except Exception:
    pass

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
EMAILJS_PRIVATE_KEY = os.getenv('EMAILJS_PRIVATE_KEY', '')


def refresh_connectivity_state(send_offline_email=True):
    """Refresh ESP32 online/offline state from heartbeat timeout."""
    if system_state['last_heartbeat'] is None:
        system_state['esp32_online'] = False
        return {
            'offline': True,
            'reason': 'no_heartbeat',
            'seconds_since_heartbeat': None,
            'threshold': HEARTBEAT_TIMEOUT,
        }

    last_beat = datetime.fromisoformat(system_state['last_heartbeat'])
    time_since = (datetime.now() - last_beat).total_seconds()
    is_offline = time_since > HEARTBEAT_TIMEOUT
    system_state['esp32_online'] = not is_offline

    should_alert = (
        is_offline
        and system_state['armed']
        and not system_state['offline_alert_sent']
        and send_offline_email
    )
    if should_alert:
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

    return {
        'offline': is_offline,
        'seconds_since_heartbeat': time_since,
        'threshold': HEARTBEAT_TIMEOUT,
    }



def send_email_alert(alert_type, subject, message, extra_data=None):
    """Send an emergency email through EmailJS."""
    if not EMAILJS_SERVICE_ID or not EMAILJS_TEMPLATE_ID or not EMAILJS_PUBLIC_KEY or not EMAILJS_TO_EMAIL:
        print('[EMAIL] Skipping alert because EmailJS is not configured')
        return False

    payload = {
        'service_id': EMAILJS_SERVICE_ID,
        'template_id': EMAILJS_TEMPLATE_ID,
        'user_id': EMAILJS_PUBLIC_KEY,
        'template_params': {  # match your EmailJS template variables
            'title': subject,
            'name': 'Sentinel Ride',
            'time': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            'message': f"{message}\n\nDetails: {json.dumps(extra_data)}",
            'email': EMAILJS_TO_EMAIL,
            'to_email': EMAILJS_TO_EMAIL,
            'device_id': system_state['device_id'],
            'armed': str(system_state['armed']),
            'timestamp': datetime.now().isoformat()
        }
    }

    if extra_data:
        payload['template_params']['details'] = json.dumps(extra_data)

    try:
        print(f'[EMAIL] Sending alert (service={EMAILJS_SERVICE_ID}, template={EMAILJS_TEMPLATE_ID})')
        # If a private key is provided (EmailJS strict mode), include it in headers and payload
        headers = {}
        if EMAILJS_PRIVATE_KEY:
            headers['X-Private-Key'] = EMAILJS_PRIVATE_KEY
            # include in payload for compatibility
            payload['private_key'] = EMAILJS_PRIVATE_KEY
            payload['accessToken'] = EMAILJS_PRIVATE_KEY
        response = requests.post(EMAILJS_API_URL, json=payload, headers=headers or None, timeout=10)
        print(f'[EMAIL] EmailJS response: {response.status_code}')
        # Always print response body for debugging
        try:
            print(f'[EMAIL] EmailJS body: {response.text}')
        except Exception:
            pass
        # Now check for errors
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
    refresh_connectivity_state(send_offline_email=True)
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
        print(f"[CRASH] Local armed state: {system_state.get('armed')} | Payload armed: {data.get('armed')}")

        if system_state['armed']:
            # Send the requested emergency message with coordinates
            result = send_email_alert(
                alert_type='crash',
                subject='Sentinel Ride: Crash detected',
                message='Alert!!!! the rider might have crashed at 17.5384240°N 78.3850000°E please respond!!!!!',
                extra_data=data
            )
            print(f"[EMAIL] send_email_alert returned: {result}")

            # (No Telegram fallback) finished handling crash alert
        
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
    state = refresh_connectivity_state(send_offline_email=True)
    return jsonify(state)


@app.errorhandler(404)
def not_found(error):
    return jsonify({'error': 'Not found'}), 404


if __name__ == '__main__':
    print("Starting Sentinel Ride Flask Server...")
    print("Dashboard: http://localhost:5001")
    app.run(debug=True, host='0.0.0.0', port=5001)
