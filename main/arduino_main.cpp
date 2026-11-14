// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Ricardo Quesada
// http://retro.moe/unijoysticle2

#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>
#include <ESP32Servo.h>
#include <uni.h>
#include "controller_callbacks.h"

extern ControllerPtr myControllers[BP32_MAX_GAMEPADS]; // BP32 library allows for up to 4 concurrent controller connections, but we only need 1

const int SERVO_PIN = 13;
const unsigned long SERVO_RUN_MS = 1000;
const int SERVO_RUN_SPEED = 180;  // full speed one direction (continuous-rotation servo)
const int SERVO_STOP_SPEED = 90;  // neutral/stopped

Servo motorServo;
bool servoRunning = false;
unsigned long servoStartMs = 0;
bool prevAPressed = false;

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

    motorServo.attach(SERVO_PIN);
    motorServo.write(SERVO_STOP_SPEED);
}

void loop() {
    vTaskDelay(1); // Ensures WDT does not get triggered when no controller is connected
    BP32.update();
    for (auto myController : myControllers) { // Only execute code when controller is connected
        if (myController && myController->isConnected() && myController->hasData()) {

            bool aPressed = myController->a();
            if (aPressed && !prevAPressed && !servoRunning) { // Start on the press, ignore if already running
                servoRunning = true;
                servoStartMs = millis();
                motorServo.write(SERVO_RUN_SPEED);
            }
            prevAPressed = aPressed;

            dumpGamepad(myController); // Prints the gamepad state, delete or comment if don't need
        }
    }

    if (servoRunning && (millis() - servoStartMs >= SERVO_RUN_MS)) {
        motorServo.write(SERVO_STOP_SPEED);
        servoRunning = false;
    }
}
