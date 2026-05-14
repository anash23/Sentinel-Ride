#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <math.h>

const char* WIFI_SSID = "011";
const char* WIFI_PASSWORD = "Avinash23";
const char* BACKEND_URL = "http://192.168.0.118:5001/api";


const uint8_t MPU_ADDR = 0x68;
const int IR_SENSOR_PIN = 35;
const int BUZZER_PIN = 25;
const int LED_PIN = 26;

const float CRASH_ACCEL_THRESHOLD_G = 2.5f;
const float CRASH_TILT_DELTA_DEG = 45.0f;
const unsigned long DROWSINESS_THRESHOLD_MS = 3000;
const unsigned long WEB_STATUS_REFRESH_MS = 2000;
const unsigned long BACKEND_HEARTBEAT_INTERVAL_MS = 5000;
const unsigned long POLL_BACKEND_STATUS_MS = 2000;
unsigned long lastBackendPoll = 0;

struct HelmetState {
    bool armed = false;
    bool crash_detected = false;
    bool drowsiness_detected = false;
    bool esp32_online = true;
    unsigned long last_heartbeat = 0;
} helmetState;

struct SensorSample {
    float accelX = 0.0f;
    float accelY = 0.0f;
    float accelZ = 0.0f;
    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
} sensorData;

bool drowsinessAlertLatched = false;
unsigned long eyeClosedStart = 0;
unsigned long lastUiUpdate = 0;
unsigned long lastBackendHeartbeatSent = 0;
float previousPitch = 0.0f;
float previousRoll = 0.0f;

void writeMPU(uint8_t reg, uint8_t value);
bool readMPU(int16_t& ax, int16_t& ay, int16_t& az, int16_t& gx, int16_t& gy, int16_t& gz);
void initMPU6050();
void connectWiFi();
void readSensors();
bool detectCrash();
bool detectDrowsiness();
void alertCrash();
void alertDrowsiness();
void sendHeartbeatToBackend();
void sendCrashToBackend();
void sendDrowsinessToBackend();

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(IR_SENSOR_PIN, INPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    Wire.begin(21, 22);
    Wire.setClock(400000);
    initMPU6050();

    connectWiFi();

    Serial.println("Sentinel Ride ready");
    Serial.printf("ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
}

void loop() {
    readSensors();

    if (helmetState.armed && detectCrash()) {
        alertCrash();
        sendCrashToBackend();
    }

    if (helmetState.armed && detectDrowsiness()) {
        alertDrowsiness();
        sendDrowsinessToBackend();
    }

    if (millis() - lastUiUpdate >= WEB_STATUS_REFRESH_MS) {
        helmetState.esp32_online = WiFi.status() == WL_CONNECTED;
        helmetState.last_heartbeat = millis();
        lastUiUpdate = millis();
    }

    // Poll backend for armed/disarmed status (Flask dashboard controls this)
    if (millis() - lastBackendPoll >= POLL_BACKEND_STATUS_MS) {
        lastBackendPoll = millis();
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            String url = String(BACKEND_URL) + "/status";
            Serial.printf("[POLL] Fetching: %s\n", url.c_str());
            if (http.begin(url)) {
                int code = http.GET();
                Serial.printf("[POLL] Response: %d\n", code);
                if (code == 200) {
                    String payload = http.getString();
                    StaticJsonDocument<256> doc;
                    DeserializationError err = deserializeJson(doc, payload);
                    if (!err) {
                        bool remoteArmed = doc["armed"] | false;
                        helmetState.armed = remoteArmed;
                        Serial.printf("[POLL] Armed status: %d\n", remoteArmed);
                    }
                }
                http.end();
            } else {
                Serial.println("[POLL] Failed to begin HTTP request");
            }
        } else {
            Serial.println("[POLL] WiFi not connected");
        }
    }

    if (helmetState.armed && WiFi.status() == WL_CONNECTED &&
        (millis() - lastBackendHeartbeatSent >= BACKEND_HEARTBEAT_INTERVAL_MS)) {
        sendHeartbeatToBackend();
        lastBackendHeartbeatSent = millis();
    }

    delay(25);
}

// Webserver removed: ESP32 runs headless and polls backend for armed state.

void writeMPU(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission(true);
}

void initMPU6050() {
    writeMPU(0x6B, 0x00);
    writeMPU(0x1B, 0x18);
    writeMPU(0x1C, 0x18);
    writeMPU(0x1A, 0x03);
}

bool readMPU(int16_t& ax, int16_t& ay, int16_t& az, int16_t& gx, int16_t& gy, int16_t& gz) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    const uint8_t count = 14;
    if (Wire.requestFrom((int)MPU_ADDR, (int)count, (int)true) != count) {
        return false;
    }

    int16_t rawTemp;
    ax = (int16_t)((Wire.read() << 8) | Wire.read());
    ay = (int16_t)((Wire.read() << 8) | Wire.read());
    az = (int16_t)((Wire.read() << 8) | Wire.read());
    rawTemp = (int16_t)((Wire.read() << 8) | Wire.read());
    gx = (int16_t)((Wire.read() << 8) | Wire.read());
    gy = (int16_t)((Wire.read() << 8) | Wire.read());
    gz = (int16_t)((Wire.read() << 8) | Wire.read());
    (void)rawTemp;
    return true;
}

void readSensors() {
    int16_t rawAx = 0, rawAy = 0, rawAz = 0;
    int16_t rawGx = 0, rawGy = 0, rawGz = 0;

    if (!readMPU(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz)) {
        return;
    }

    sensorData.accelX = rawAx / 2048.0f;
    sensorData.accelY = rawAy / 2048.0f;
    sensorData.accelZ = rawAz / 2048.0f;
    sensorData.gyroX = rawGx / 16.4f;
    sensorData.gyroY = rawGy / 16.4f;
    sensorData.gyroZ = rawGz / 16.4f;

    sensorData.pitch = atan2f(sensorData.accelY, sqrtf(sensorData.accelX * sensorData.accelX + sensorData.accelZ * sensorData.accelZ)) * 57.2958f;
    sensorData.roll = atan2f(-sensorData.accelX, sensorData.accelZ) * 57.2958f;
}

bool detectCrash() {
    const float accelMagnitude = sqrtf(
        sensorData.accelX * sensorData.accelX +
        sensorData.accelY * sensorData.accelY +
        sensorData.accelZ * sensorData.accelZ
    );

    const float pitchDelta = fabsf(sensorData.pitch - previousPitch);
    const float rollDelta = fabsf(sensorData.roll - previousRoll);

    previousPitch = sensorData.pitch;
    previousRoll = sensorData.roll;

    return accelMagnitude >= CRASH_ACCEL_THRESHOLD_G && (pitchDelta >= CRASH_TILT_DELTA_DEG || rollDelta >= CRASH_TILT_DELTA_DEG);
}

bool detectDrowsiness() {
    const int irValue = digitalRead(IR_SENSOR_PIN);

    if (irValue == LOW) {
        if (eyeClosedStart == 0) {
            eyeClosedStart = millis();
        }

        if (millis() - eyeClosedStart >= DROWSINESS_THRESHOLD_MS) {
            return true;
        }
    } else {
        eyeClosedStart = 0;
        drowsinessAlertLatched = false;
    }

    return false;
}

void alertCrash() {
    helmetState.crash_detected = true;
    digitalWrite(LED_PIN, HIGH);
    for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(120);
        digitalWrite(BUZZER_PIN, LOW);
        delay(120);
    }
    digitalWrite(LED_PIN, LOW);
}

void alertDrowsiness() {
    if (drowsinessAlertLatched) {
        return;
    }

    helmetState.drowsiness_detected = true;
    drowsinessAlertLatched = true;
    for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(180);
        digitalWrite(BUZZER_PIN, LOW);
        delay(180);
    }
}

void sendHeartbeatToBackend() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    HTTPClient http;
    String url = String(BACKEND_URL) + "/heartbeat";
    Serial.printf("[HB] Sending to: %s\n", url.c_str());
    if (!http.begin(url)) {
        Serial.println("[HB] Failed to begin HTTP");
        return;
    }

    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<256> doc;
    doc["status"] = "alive";
    doc["armed"] = helmetState.armed;
    doc["crash"] = helmetState.crash_detected;
    doc["drowsiness"] = helmetState.drowsiness_detected;
    doc["ip"] = WiFi.localIP().toString();

    String body;
    serializeJson(doc, body);
    int code = http.POST(body);
    Serial.printf("[HB] Response: %d\n", code);
    http.end();
}

void sendCrashToBackend() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    HTTPClient http;
    String url = String(BACKEND_URL) + "/crash";
    if (!http.begin(url)) {
        return;
    }

    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<256> doc;
    doc["alert"] = "crash";
    doc["armed"] = helmetState.armed;
    doc["accel_x"] = sensorData.accelX;
    doc["accel_y"] = sensorData.accelY;
    doc["accel_z"] = sensorData.accelZ;
    doc["pitch"] = sensorData.pitch;
    doc["roll"] = sensorData.roll;

    String body;
    serializeJson(doc, body);
    http.POST(body);
    http.end();
}

void sendDrowsinessToBackend() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    HTTPClient http;
    String url = String(BACKEND_URL) + "/drowsiness";
    if (!http.begin(url)) {
        return;
    }

    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<256> doc;
    doc["alert"] = "drowsiness";
    doc["armed"] = helmetState.armed;
    doc["ir_state"] = digitalRead(IR_SENSOR_PIN);

    String body;
    serializeJson(doc, body);
    http.POST(body);
    http.end();
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("WiFi connection failed; AP mode is not enabled in this sketch.");
    }
}
