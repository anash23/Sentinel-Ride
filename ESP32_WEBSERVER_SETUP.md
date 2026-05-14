# 🛡️ Sentinel Ride — ESP32 Web Server Setup

## Overview

The ESP32 now hosts its own **control panel webpage** for ARM/DISARM functions. No need to connect to Flask server for controlling the helmet system.

---

## New Features

✅ **Web Server Running on ESP32** — Access dashboard directly from helmet's IP  
✅ **ARM/DISARM Controls** — Control directly from ESP32  
✅ **Real-time Status Display** — Crash, drowsiness, system state  
✅ **Activity Log** — Live event tracking  
✅ **Optional Backend Sync** — Can still send alerts to Flask server  

---

## Installation

### 1. Edit WiFi Credentials

Open `esp32/smart_helmet_with_webserver.ino` and configure:

```cpp
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
```

### 2. (Optional) Configure Backend Server

If you want to also sync with Flask backend:

```cpp
const char* BACKEND_URL = "http://192.168.1.100:5001/api";
```

Or leave it as-is to use the web server standalone.

### 3. Install Required Libraries in Arduino IDE

- **WiFi** (built-in)
- **WebServer** (built-in)
- **Wire** (built-in)
- **MPU6050** (by Electronic Cats)
- **ArduinoJson** (by Benoit Blanchon)

### 4. Upload to ESP32

1. Select **Board:** ESP32 Dev Module
2. Select **Upload Speed:** 921600
3. Click **Upload**

---

## Usage

### 1. Power On ESP32

The firmware will:
- Initialize MPU6050 sensor
- Connect to WiFi
- Start web server

### 2. Check Serial Output

Open Serial Monitor (115200 baud) to see:

```
🛡️ Sentinel Ride — ESP32 Web Server
Initializing...

✅ MPU6050 initialized
Connecting to WiFi: YOUR_SSID
✅ WiFi connected!
📱 IP: 192.168.1.150

✅ Web server started
🚀 System ready!
📱 Open: http://192.168.1.150/
```

### 3. Open Control Panel

Open your browser and go to:

```
http://<ESP32_IP>/
```

Example: `http://192.168.1.150/`

### 4. Use the Dashboard

- **View Status:** System armed/disarmed, crash detection, drowsiness alerts
- **ARM Button:** Start monitoring
- **DISARM Button:** Stop monitoring
- **Activity Log:** Real-time events

---

## Web Server Endpoints

| Endpoint | Method | Response |
|----------|--------|----------|
| `/` | GET | HTML dashboard |
| `/api/status` | GET | JSON status |
| `/api/arm` | POST | Arm system |
| `/api/disarm` | POST | Disarm system |

### Example Status Response

```json
{
    "armed": true,
    "crash_detected": false,
    "drowsiness_detected": false,
    "esp32_online": true,
    "ip": "192.168.1.150"
}
```

---

## Architecture Now

```
ESP32 (All-in-One)
├─ MPU6050 (Sensors)
├─ Web Server (Control Panel)
├─ WiFi (Connectivity)
└─ (Optional) Backend Sync

Browser
└─ http://192.168.1.150/
```

**No Flask server needed for basic control** — just access ESP32 directly!

---

## Customization

### Change Web Server Port

```cpp
WebServer server(80);  // Change to any port (1024-65535)
```

### Adjust Crash Threshold

```cpp
const float CRASH_ACCEL_THRESHOLD = 2.5;  // G-force
const float CRASH_TILT_THRESHOLD = 45;    // Rotation speed
```

### Adjust Drowsiness Threshold

```cpp
const int DROWSINESS_THRESHOLD_MS = 3000;  // 3 seconds
```

### Change Backend Sync Interval

```cpp
const int BACKEND_SYNC_INTERVAL = 5000;  // 5 seconds
```

---

## Troubleshooting

### ESP32 Won't Connect to WiFi

- Check SSID and password are correct
- Verify WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Check if WiFi is nearby

### Can't Access Dashboard

- Verify ESP32 IP address in Serial Monitor
- Try `http://192.168.1.150/` (replace with actual IP)
- Make sure you're on the same WiFi network
- Check if firewall is blocking port 80

### Sensors Not Working

- Check I2C connections (SDA=21, SCL=22)
- Verify pull-up resistors (4.7kΩ typical)
- Check MPU6050 power (3.3V)

### Backend Not Receiving Data

- Update BACKEND_URL to your Flask server IP
- Make sure Flask server is running on port 5001
- Check if firewall allows outgoing connections

---

## Two Options

### Option 1: ESP32 Standalone (Recommended)
- ✅ No Flask needed
- ✅ Access directly: `http://ESP32_IP/`
- ✅ Control helmet locally
- ❌ No cloud logging

### Option 2: ESP32 + Flask Backend
- ✅ Local control on ESP32
- ✅ Backend syncing for logging
- ✅ Email alerts
- ✅ Remote dashboard
- ⚠️ Requires Flask server running

---

## Next Steps

1. Upload `smart_helmet_with_webserver.ino` to ESP32
2. Configure WiFi credentials
3. Power on ESP32
4. Access control panel at `http://ESP32_IP/`
5. Test ARM/DISARM functions
6. Simulate crash/drowsiness events

🚀 **Your smart helmet control panel is now on the helmet itself!**
