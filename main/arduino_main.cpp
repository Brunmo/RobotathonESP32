// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Ricardo Quesada
// http://retro.moe/unijoysticle2

#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>
#include <uni.h>
#include "controller_callbacks.h"

#include <Wire.h>
#include <Arduino_APDS9960.h>
#include <bits/stdc++.h>

#define APDS9960_INT 2
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_FREQ 100000

TwoWire I2C_0 = TwoWire(0);
APDS9960 apds = APDS9960(I2C_0, APDS9960_INT);

extern ControllerPtr myControllers[BP32_MAX_GAMEPADS]; // BP32 library allows for up to 4 concurrent controller connections, but we only need 1

// ---- Drivetrain pins: DRV8833 dual H-bridge, per the Robotathon hardware guide. ----
// Each motor takes two PWM-capable pins (IN1/IN2): PWM one side for a direction, the
// other side held low. Both low = coast, both high = brake (we don't use brake here).
// Left motor
const int LEFT_IN1_PIN = 16;
const int LEFT_IN2_PIN = 17;
const int LEFT_IN1_CHANNEL = 0;
const int LEFT_IN2_CHANNEL = 1;
// Right motor
const int RIGHT_IN1_PIN = 18;
const int RIGHT_IN2_PIN = 19;
const int RIGHT_IN1_CHANNEL = 2;
const int RIGHT_IN2_CHANNEL = 3;

const int PWM_FREQ_HZ = 5000;
const int PWM_RESOLUTION_BITS = 8;  // duty range 0-255

const int DRIVE_SPEED = 255;  // full speed, either direction

void driveMotor(int in1Channel, int in2Channel, int speed) {
    // speed: -255 (full reverse) .. 255 (full forward)
    speed = constrain(speed, -255, 255);
    if (speed > 0) {
        ledcWrite(in1Channel, speed);
        ledcWrite(in2Channel, 0);
    } else if (speed < 0) {
        ledcWrite(in1Channel, 0);
        ledcWrite(in2Channel, -speed);
    } else {
        ledcWrite(in1Channel, 0);
        ledcWrite(in2Channel, 0);
    }
}

void dumpGamepad(ControllerPtr ctl) {
    Console.printf(
        "DPAD: %d A: %d B: %d X: %d Y: %d LX: %d LY: %d RX: %d RY: %d L1: %d R1: %d L2: %d R2: %d\n",
        ctl->dpad(),        // D-pad
        ctl->a(),           // Letter buttons
        ctl->b(),
        ctl->x(),
        ctl->y(),
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->l1(),           // Bumpers
        ctl->r1(),
        ctl->l2(),
        ctl->r2()
    );
}

void setup() {
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys();
    esp_log_level_set("gpio", ESP_LOG_ERROR); // Suppress info log spam from gpio_isr_service
    uni_bt_allowlist_set_enabled(true);

    ledcSetup(LEFT_IN1_CHANNEL, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(LEFT_IN1_PIN, LEFT_IN1_CHANNEL);
    ledcSetup(LEFT_IN2_CHANNEL, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(LEFT_IN2_PIN, LEFT_IN2_CHANNEL);

    ledcSetup(RIGHT_IN1_CHANNEL, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(RIGHT_IN1_PIN, RIGHT_IN1_CHANNEL);
    ledcSetup(RIGHT_IN2_CHANNEL, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(RIGHT_IN2_PIN, RIGHT_IN2_CHANNEL);

    //sets up I2C protocol
    I2C_0.begin(I2C_SDA, I2C_SCL, I2C_FREQ);

    //sets up color sensor
    apds.setInterruptPin(APDS9960_INT);
    apds.begin();
    Serial.begin(115200);
}

void loop() {
    vTaskDelay(1); // Ensures WDT does not get triggered when no controller is connected
    BP32.update();
    for (auto myController : myControllers) { // Only execute code when controller is connected
        if (myController && myController->isConnected() && myController->hasData()) {

            int speed = 0;
            if (myController->x()) {
                speed = DRIVE_SPEED;    // Hold X to move forward
            } else if (myController->b()) {
                speed = -DRIVE_SPEED;   // Hold B to move backward
            }

            driveMotor(LEFT_IN1_CHANNEL, LEFT_IN2_CHANNEL, speed);
            driveMotor(RIGHT_IN1_CHANNEL, RIGHT_IN2_CHANNEL, speed);

            dumpGamepad(myController); // Prints the gamepad state, delete or comment if don't need
        }
    }
}
