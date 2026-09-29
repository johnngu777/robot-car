/*
 * ESP32 main control unit.
 * - Receives {rx,ry,rheading,tx,ty,push} JSON over UDP from the MaixCam (see
 *   docs/PROTOCOL.md).
 * - Mixes that into 4 omniwheel motor commands (mecanum-style kinematics).
 * - Falls back to local-only behavior if no packet arrives for
 *   LAST_PACKET_TIMEOUT_MS (WiFi dropout risk from slide 9).
 * - Reads the IMU for orientation as a sanity check / fallback heading source.
 *
 * Wire the // TODOs in include/config.h before this will actually drive.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "config.h"

WiFiUDP udp;
uint8_t packetBuffer[512];

unsigned long lastPacketMs = 0;
float lastTx = 0, lastTy = 0, lastRx = 0, lastRy = 0;
bool lastPush = false;

// ---------------------------------------------------------- motor setup ---
static const MotorPins MOTORS[4] = {MOTOR_FL, MOTOR_FR, MOTOR_RL, MOTOR_RR};
static const int PWM_CHANNELS[4] = {0, 1, 2, 3};

void setupMotors() {
    for (int i = 0; i < 4; i++) {
        pinMode(MOTORS[i].in1, OUTPUT);
        pinMode(MOTORS[i].in2, OUTPUT);
        ledcSetup(PWM_CHANNELS[i], PWM_FREQ_HZ, PWM_RESOLUTION);
        ledcAttachPin(MOTORS[i].pwm, PWM_CHANNELS[i]);
        pinMode(MOTORS[i].enc_a, INPUT);
        pinMode(MOTORS[i].enc_b, INPUT);
        // TODO: attachInterrupt on enc_a for real quadrature counting once
        // you need closed-loop speed control instead of open-loop PWM.
    }
}

// speed in [-1.0, 1.0]
void driveMotor(int idx, float speed) {
    speed = constrain(speed, -1.0f, 1.0f);
    bool forward = speed >= 0;
    digitalWrite(MOTORS[idx].in1, forward ? HIGH : LOW);
    digitalWrite(MOTORS[idx].in2, forward ? LOW : HIGH);
    int duty = (int)(fabs(speed) * ((1 << PWM_RESOLUTION) - 1));
    ledcWrite(PWM_CHANNELS[idx], duty);
}

void stopAllMotors() {
    for (int i = 0; i < 4; i++) driveMotor(i, 0.0f);
}

/*
 * Mecanum/omniwheel mixing: vx = strafe (+right), vy = forward (+away from
 * you), w = rotation (+clockwise from above). Signs depend on your physical
 * wheel roller orientation — if the robot drives diagonally when you ask for
 * straight forward, flip the sign on vx or w here, not in the vision code.
 */
void driveOmni(float vx, float vy, float w) {
    float fl = vy + vx + w;
    float fr = vy - vx - w;
    float rl = vy - vx + w;
    float rr = vy + vx - w;

    float maxMag = max(1.0f, max(max(fabs(fl), fabs(fr)), max(fabs(rl), fabs(rr))));
    driveMotor(0, fl / maxMag);
    driveMotor(1, fr / maxMag);
    driveMotor(2, rl / maxMag);
    driveMotor(3, rr / maxMag);
}

// ----------------------------------------------------------------- IMU ---
void setupImu() {
    // TODO: Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN) + init your specific IMU chip
    // (MPU6050 is the common cheap choice). Left unimplemented until the
    // exact part is picked — see esp32/platformio.ini lib_deps for candidates.
}

float readImuHeadingDegrees() {
    // TODO: integrate gyro Z, or read a fused orientation if your IMU library
    // provides one. Returning 0 is a placeholder so the rest of the loop compiles.
    return 0.0f;
}

// --------------------------------------------------------------- WiFi ---
void setupWifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected. ESP32 IP (set this in maixcam/vision.py): ");
    Serial.println(WiFi.localIP());
    udp.begin(UDP_PORT);
}

void pollUdp() {
    int packetSize = udp.parsePacket();
    if (packetSize <= 0) return;

    int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len <= 0) return;
    packetBuffer[len] = 0;

    JsonDocument doc;  // ArduinoJson v7
    DeserializationError err = deserializeJson(doc, packetBuffer, len);
    if (err) {
        Serial.print("JSON parse failed: ");
        Serial.println(err.c_str());
        return;
    }

    lastRx = doc["rx"] | 0.0f;
    lastRy = doc["ry"] | 0.0f;
    lastTx = doc["tx"] | 0.0f;
    lastTy = doc["ty"] | 0.0f;
    lastPush = doc["push"] | false;
    lastPacketMs = millis();
}

// ------------------------------------------------------- control loop ---
void runNormalControl() {
    // Simple proportional drive toward (tx, ty) in field-normalized coords.
    // TODO: replace with something that uses rheading/IMU heading to turn-then-drive,
    // or a proper feedback controller, once this basic version is working end to end.
    float dx = lastTx - lastRx;
    float dy = lastTy - lastRy;

    float vx = constrain(dx * 2.0f, -1.0f, 1.0f);
    float vy = constrain(dy * 2.0f, -1.0f, 1.0f);
    float w = 0.0f;  // TODO: use heading error once real heading is available

    if (lastPush) {
        // Driving through a block: prioritize straight-line torque over precision.
        vx *= 0.5f;
    }

    driveOmni(vx, vy, w);
}

void runFallbackControl() {
    // WiFi dropout fallback (slide 9). Placeholder: stop in place, which is
    // safe but passive. A slow forward sweep using only the IMU is a
    // reasonable next step once heading is real.
    stopAllMotors();
}

void setup() {
    Serial.begin(115200);
    setupMotors();
    setupImu();
    setupWifi();
}

void loop() {
    pollUdp();

    if (millis() - lastPacketMs < LAST_PACKET_TIMEOUT_MS) {
        runNormalControl();
    } else {
        runFallbackControl();
    }
}
