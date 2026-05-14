# Sentinel Ride — Smart Helmet Safety System

## Project Overview

Breadboard-based smart helmet safety prototype with:
- ESP32-based crash detection (MPU6050)
- Drowsiness detection (IR sensor)
- Emergency email alerts (EmailJS)
- Flask web dashboard
- ARM/DISARM monitoring control

## Development Stack

- **ESP32 Firmware**: Arduino IDE (C/Arduino)
- **Backend**: Python Flask
- **Frontend**: HTML/CSS/JavaScript
- **Alerts**: EmailJS

## Setup Instructions

### Backend (Flask Server)

```bash
cd backend
pip install -r requirements.txt
python app.py
```

Server runs on `http://localhost:5000`

### Frontend

Open `frontend/index.html` in browser or serve via Flask static files.

### ESP32 Firmware

1. Install Arduino IDE
2. Add ESP32 board support
3. Install required libraries: MPU6050, WiFi
4. Open `esp32/smart_helmet.ino` in Arduino IDE
5. Configure WiFi credentials in sketch
6. Upload to ESP32

## Key Communication

- ESP32 → Flask: POST requests (heartbeat, crash, drowsiness)
- Flask ↔ Dashboard: WebSocket or polling
- Dashboard → EmailJS: Trigger emergency alerts

## Priority Features

1. ESP32 ↔ Flask communication
2. Crash detection
3. Dashboard display
4. EmailJS integration
5. Drowsiness detection
6. Offline timeout handling
