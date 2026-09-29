#pragma once

// ---------------------------------------------------------------- WiFi ---
#define WIFI_SSID     "TODO_your_ssid"
#define WIFI_PASSWORD "TODO_your_password"
#define UDP_PORT      8888   // must match maixcam/vision.py's ESP32_PORT

// If WiFi drops for this long, stop trusting the last command and run local
// fallback logic instead (see slide 9: WiFi dropout risk).
#define LAST_PACKET_TIMEOUT_MS 300

// --------------------------------------------------------- motor pins ---
// 4x omniwheels, encoded DC motors. Fill in your actual GPIO numbers —
// these are placeholders. Each motor needs 2 direction pins + 1 PWM pin
// (or fewer if your driver board takes PWM+DIR instead of PWM+PWM).
struct MotorPins {
    int pwm;
    int in1;
    int in2;
    int enc_a;
    int enc_b;
};

// Layout convention: looking down at the robot from above, FL/FR/RL/RR.
// Match this physically or your mixing math in main.cpp will drive sideways.
static const MotorPins MOTOR_FL = {/*pwm*/ 25, /*in1*/ 26, /*in2*/ 27, /*enc_a*/ 34, /*enc_b*/ 35};
static const MotorPins MOTOR_FR = {/*pwm*/ 14, /*in1*/ 12, /*in2*/ 13, /*enc_a*/ 36, /*enc_b*/ 39};
static const MotorPins MOTOR_RL = {/*pwm*/ 33, /*in1*/ 32, /*in2*/ 15, /*enc_a*/  4, /*enc_b*/  5};
static const MotorPins MOTOR_RR = {/*pwm*/ 18, /*in1*/ 19, /*in2*/ 21, /*enc_a*/ 22, /*enc_b*/ 23};

// PWM
#define PWM_FREQ_HZ   20000
#define PWM_RESOLUTION 8      // bits -> duty range 0-255

// --------------------------------------------------------------- IMU ---
#define IMU_SDA_PIN 21   // TODO: confirm — may collide with motor pins above,
#define IMU_SCL_PIN 22   // re-pin once your wiring diagram is final
