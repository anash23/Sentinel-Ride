/*
 * Sentinel Ride — Smart Helmet Safety System
 * ESP32 Firmware for MPU6050 Crash Detection & IR Sleep Detection
 * 
 * Hardware:
 * - ESP32 DevKit
 * - MPU6050 (I2C: SDA=21, SCL=22)
 * - IR Obstacle Sensor (GPIO 35)
 * - Buzzer (GPIO 25)
 * - LED (GPIO 26)
 * 
 * Sends heartbeat + status to Flask server every 5 seconds
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <MPU6050.h>
#include <ArduinoJson.h>

// WiFi Credentials
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// Server Configuration
const char* SERVER_URL = "http://192.168.1.100:5000/api";
const int HEARTBEAT_INTERVAL = 5000; // 5 seconds

// Pin Configuration
const int IR_SENSOR_PIN = 35;
const int BUZZER_PIN = 25;
const int LED_PIN = 26;

// Crash Detection Thresholds
const float CRASH_ACCEL_THRESHOLD = 2.5; // G-force
const float CRASH_TILT_THRESHOLD = 45;   // degrees

// Drowsiness Detection
const int DROWSINESS_THRESHOLD_MS = 3000; // 3 seconds
unsigned long eye_closed_start = 0;
bool drowsiness_alert_sent = false;

// MPU6050 Instance
MPU6050 mpu;

// State Tracking
struct SensorData {
    float accelX, accelY, accelZ;
    float gyroX, gyroY, gyroZ;
    float temperature;
    bool crash_detected;
    bool drowsiness_detected;
} sensor_data;

unsigned long last_heartbeat = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize Pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("\n🛡️ Sentinel Ride Helmet Safety System");
    Serial.println("Initializing...\n");
    
    // Initialize I2C for MPU6050
    Wire.begin(21, 22); // SDA=21, SCL=22
    delay(100);
    
    // Initialize MPU6050
    if (!mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_16G)) {
        Serial.println("❌ MPU6050 initialization failed!");
        while (1) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            delay(100);
        }
    }
    Serial.println("✅ MPU6050 initialized");
    
    // Connect to WiFi
    connectWiFi();
    
    Serial.println("🚀 System ready for operation");
}

void loop() {
    // Read sensor data
    readSensors();
    
    // Detect crashes
    sensor_data.crash_detected = detectCrash();
    if (sensor_data.crash_detected) {
        alertCrash();
    }
    
    // Detect drowsiness/sleep
    sensor_data.drowsiness_detected = detectDrowsiness();
    if (sensor_data.drowsiness_detected) {
        alertDrowsiness();
    }
    
    // Send heartbeat
    if (millis() - last_heartbeat >= HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        last_heartbeat = millis();
    }
    
    delay(100);
}

/**
 * Read sensor data from MPU6050
 */
void readSensors() {
    Vector rawAccel = mpu.readRawAccel();
    Vector rawGyro = mpu.readRawGyro();
    
    // Convert to G-force and degrees/sec
    sensor_data.accelX = rawAccel.XAxis / 2048.0;
    sensor_data.accelY = rawAccel.YAxis / 2048.0;
    sensor_data.accelZ = rawAccel.ZAxis / 2048.0;
    
    sensor_data.gyroX = rawGyro.XAxis / 131.0;
    sensor_data.gyroY = rawGyro.YAxis / 131.0;
    sensor_data.gyroZ = rawGyro.ZAxis / 131.0;
    
    sensor_data.temperature = mpu.readTemperature();
}

/**
 * Detect crash using accelerometer + gyroscope
 */
bool detectCrash() {
    float total_accel = sqrt(
        sensor_data.accelX * sensor_data.accelX +
        sensor_data.accelY * sensor_data.accelY +
        sensor_data.accelZ * sensor_data.accelZ
    );
    
    float gyro_magnitude = sqrt(
        sensor_data.gyroX * sensor_data.gyroX +
        sensor_data.gyroY * sensor_data.gyroY +
        sensor_data.gyroZ * sensor_data.gyroZ
    );
    
    // Crash = high acceleration + high rotation
    return (total_accel > CRASH_ACCEL_THRESHOLD) && (gyro_magnitude > 100);
}

/**
 * Detect drowsiness using IR sensor
 */
bool detectDrowsiness() {
    int ir_value = digitalRead(IR_SENSOR_PIN);
    
    // IR sensor HIGH = no obstruction (eyes open)
    // IR sensor LOW = obstruction (eyes closed)
    
    if (ir_value == LOW) {
        // Eyes closed
        if (eye_closed_start == 0) {
            eye_closed_start = millis();
            drowsiness_alert_sent = false;
        }
        
        unsigned long eyes_closed_duration = millis() - eye_closed_start;
        
        if (eyes_closed_duration > DROWSINESS_THRESHOLD_MS) {
            return true;
        }
    } else {
        // Eyes open
        eye_closed_start = 0;
        drowsiness_alert_sent = false;
    }
    
    return false;
}

/**
 * Alert for crash
 */
void alertCrash() {
    Serial.println("🚨 CRASH DETECTED!");
    
    // Buzzer pattern: rapid beeps
    for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    
    // Send crash alert to server
    sendCrashAlert();
}

/**
 * Alert for drowsiness
 */
void alertDrowsiness() {
    if (!drowsiness_alert_sent) {
        Serial.println("⚠️ DROWSINESS DETECTED!");
        
        // Buzzer pattern: slow beeps
        for (int i = 0; i < 3; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
        
        drowsiness_alert_sent = true;
        sendDrowsinessAlert();
    }
}

/**
 * Send heartbeat to Flask server
 */
void sendHeartbeat() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ WiFi disconnected, attempting reconnect...");
        connectWiFi();
        return;
    }
    
    HTTPClient http;
    http.begin(String(SERVER_URL) + "/heartbeat");
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<200> doc;
    doc["status"] = "alive";
    doc["timestamp"] = millis();
    doc["rssi"] = WiFi.RSSI();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    int httpCode = http.POST(jsonString);
    
    if (httpCode == 200) {
        Serial.println("💓 Heartbeat sent");
    } else {
        Serial.printf("❌ Heartbeat failed: %d\n", httpCode);
    }
    
    http.end();
}

/**
 * Send crash alert to Flask server
 */
void sendCrashAlert() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    http.begin(String(SERVER_URL) + "/crash");
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<300> doc;
    doc["alert"] = "crash";
    doc["timestamp"] = millis();
    doc["accel_x"] = sensor_data.accelX;
    doc["accel_y"] = sensor_data.accelY;
    doc["accel_z"] = sensor_data.accelZ;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    int httpCode = http.POST(jsonString);
    Serial.printf("🚨 Crash alert sent (HTTP %d)\n", httpCode);
    
    http.end();
}

/**
 * Send drowsiness alert to Flask server
 */
void sendDrowsinessAlert() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    http.begin(String(SERVER_URL) + "/drowsiness");
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<300> doc;
    doc["alert"] = "drowsiness";
    doc["timestamp"] = millis();
    doc["ir_reading"] = digitalRead(IR_SENSOR_PIN);
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    int httpCode = http.POST(jsonString);
    Serial.printf("⚠️ Drowsiness alert sent (HTTP %d)\n", httpCode);
    
    http.end();
}

/**
 * Connect to WiFi
 */
void connectWiFi() {
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n✅ WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n❌ WiFi connection failed");
    }
}
