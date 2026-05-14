# 🛡️ Sentinel Ride — Smart Helmet Safety System

A breadboard-based smart helmet safety prototype using ESP32 with crash detection, drowsiness monitoring, and emergency alerts.

## Features

✅ **Crash Detection** — MPU6050 accelerometer + gyroscope  
✅ **Drowsiness Detection** — IR obstacle sensor  
✅ **Emergency Alerts** — EmailJS integration  
✅ **Web Dashboard** — Real-time monitoring & control  
✅ **Heartbeat Monitoring** — Device connectivity tracking  
✅ **ARM/DISARM System** — User-controlled activation  

## System Architecture

```
IR Sensor + MPU6050
        ↓
      ESP32 ────────→ Flask Server ────→ Web Dashboard
        ↓                                    ↓
                                         EmailJS
```

## Hardware Requirements

| Component       | Purpose              |
|-----------------|----------------------|
| ESP32 DevKit    | Main controller      |
| MPU6050         | Crash detection      |
| IR Sensor       | Sleep detection      |
| Buzzer          | Audio alerts         |
| LED             | Visual indication    |
| Breadboard      | Prototype setup      |

## Pin Configuration (ESP32)

| Component  | GPIO Pin |
|-----------|----------|
| MPU6050 SDA  | GPIO 21  |
| MPU6050 SCL  | GPIO 22  |
| IR Sensor    | GPIO 35  |
| Buzzer       | GPIO 25  |
| LED          | GPIO 26  |

## Software Setup

### Backend (Flask Server)

```bash
cd backend
pip install -r requirements.txt
python app.py
```

Server runs on: `http://localhost:5000`

### Frontend

The dashboard is served automatically by Flask at `http://localhost:5000`

### ESP32 Firmware

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support (via Board Manager)
3. Install required libraries:
   - MPU6050 (by Electronic Cats)
   - ArduinoJson

4. Open `esp32/smart_helmet.ino` and configure:
   ```cpp
   const char* WIFI_SSID = "YOUR_SSID";
   const char* WIFI_PASSWORD = "YOUR_PASSWORD";
   const char* SERVER_URL = "http://YOUR_SERVER_IP:5000/api";
   ```

5. Upload to ESP32

## Configuration

### Environment Variables

Create `backend/.env` from `.env.example`:

```env
FLASK_ENV=development
ESP32_IP=192.168.1.100
HEARTBEAT_TIMEOUT=25
EMAILJS_SERVICE_ID=your_service_id
EMAILJS_TEMPLATE_ID=your_template_id
EMAILJS_PUBLIC_KEY=your_public_key
EMAILJS_TO_EMAIL=recipient@example.com
```

### Thresholds (Adjustable)

| Parameter | Value | File |
|-----------|-------|------|
| Crash acceleration threshold | 2.5G | `esp32/smart_helmet.ino` |
| Crash rotation threshold | 100°/s | `esp32/smart_helmet.ino` |
| Drowsiness threshold | 3 seconds | `backend/app.py` |
| Offline timeout | 25 seconds | `backend/app.py` |
| Heartbeat interval | 5 seconds | `esp32/smart_helmet.ino` |

## API Endpoints

### Dashboard Status

```
GET /api/status
```

Returns current system state.

### ARM/DISARM

```
POST /api/arm
POST /api/disarm
```

### ESP32 Communication

```
POST /api/heartbeat      — Device alive signal
POST /api/crash          — Crash detection alert
POST /api/drowsiness     — Drowsiness detection alert
```

## Demo Workflow

### 1. Start System
```bash
python backend/app.py
```

Open `http://localhost:5000` in browser.

### 2. Upload ESP32 Firmware
- Configure WiFi credentials
- Upload `smart_helmet.ino` to ESP32
- Device automatically connects and sends heartbeats

### 3. ARM System
Click **[ARM]** button on dashboard.

### 4. Test Drowsiness Detection
Cover IR sensor → Buzzer sounds → Dashboard shows alert

### 5. Test Crash Detection
Shake/tilt the breadboard → Buzzer activates → Emergency email triggered

## Troubleshooting

### ESP32 Not Connecting to WiFi
- Check SSID and password
- Verify ESP32 is in range
- Check router 2.4GHz band is enabled

### Dashboard Shows Device OFFLINE
- Verify ESP32 is powered on
- Check Flask server is running
- Confirm firewall isn't blocking port 5000

### Crash Detection Too Sensitive
- Increase `CRASH_ACCEL_THRESHOLD` in `smart_helmet.ino`
- Adjust `CRASH_TILT_THRESHOLD`

### EmailJS Alerts Not Sending
- Verify EmailJS credentials in `.env`
- Check email template exists in EmailJS account
- Verify sender email is authorized

## Development Priorities

1. ✅ ESP32 ↔ Flask communication
2. ✅ Crash detection (MPU6050)
3. ✅ Dashboard UI
4. ⏳ EmailJS integration
5. ⏳ Drowsiness detection refinement
6. ⏳ Offline timeout logic
7. ⏳ UI polishing

## File Structure

```
Sentinel-Ride/
├── backend/
│   ├── app.py              # Flask server
│   ├── requirements.txt     # Python dependencies
│   └── .env.example         # Configuration template
├── frontend/
│   ├── index.html           # Dashboard UI
│   ├── styles.css           # Dashboard styling
│   └── app.js               # Frontend logic
├── esp32/
│   └── smart_helmet.ino     # ESP32 firmware
├── README.md                # This file
└── .github/
    └── copilot-instructions.md
```

## Future Enhancements

- 🔄 WebSocket support for real-time updates
- 📍 WiFi-based geolocation for alerts
- 📊 Dashboard analytics and history
- 🔐 Authentication for web dashboard
- 📱 Mobile app integration
- 🎯 Machine learning for crash detection

## License

MIT License — Open for educational and commercial use.

---

**Built with ❤️ for rider safety**
