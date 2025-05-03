#include "NormalStateHandler.h"

void NormalStateHandler::handle(DeviceSystem& system) {
    ButtonManager& deviceButton = system.getDeviceButton();
    ButtonManager& actionButton = system.getActionButton();
    LedManager& leds = system.getLeds();
    SoundManager& sound = system.getSound();

    bool devicePressed = deviceButton.isPressed();
    bool deviceJustPressed = deviceButton.wasJustPressed();
    system.isDeviceButtonCurrentlyPressed() = devicePressed;

    if (devicePressed) {
        system.getDeviceButtonReleaseTime() = millis();

        if (deviceJustPressed) {
            system.isDisplayCounter() = true;
            leds.animateCounterLeds(system.getCounter());
        }

        if (!system.isActionDoneThisCycle() && actionButton.wasJustPressed()) {
            system.isActionInProgress() = true;
            system.getActionStartTime() = millis();
            leds.setActionStarted(true);
        }
    } else {
        if (millis() - system.getDeviceButtonReleaseTime() >= DISPLAY_TIMEOUT_MS) {
            system.isDisplayCounter() = false;
            leds.turnOffCounterLeds();
        }
        if (system.isActionInProgress()) {
            system.isActionInProgress() = false;
            leds.setActionStarted(false);
            leds.setActionCompleted(false);
            sound.stopTone();
            system.isPendingCounterIncrement() = false;
            Serial.println("Action cancelled");
        }
        system.isActionDoneThisCycle() = false;
    }

    if (system.isActionInProgress()) {
        unsigned long heldDuration = millis() - system.getActionStartTime();
        float progress = min((float)heldDuration / ACTION_HOLD_TIME_MS, 1.0f);
        sound.playSweepTone(progress);

        if (!actionButton.isPressed()) {
            system.isActionInProgress() = false;
            leds.setActionStarted(false);
            leds.setActionCompleted(false);
            sound.stopTone();
            system.isPendingCounterIncrement() = false;
            Serial.println("Action cancelled");
        }

        if (heldDuration >= ACTION_HOLD_TIME_MS && !system.isActionDoneThisCycle()) {
            system.isActionDoneThisCycle() = true;
            system.isPendingCounterIncrement() = true;
            system.isActionInProgress() = false;

            leds.setActionStarted(false);
            leds.setActionCompleted(true);
            sound.stopTone();
            sound.playConfirmationBeep();
            Serial.println("Action completed");
        }
    }

    if (actionButton.wasJustReleased()) {
        leds.setActionCompleted(false);
        sound.stopTone();
        system.isActionInProgress() = false;

        if (system.isPendingCounterIncrement() && system.getCounter() < MAX_COUNTER) {
            system.getCounter()++;
            system.isPendingCounterIncrement() = false;
            system.updateDisplay();
            Serial.print("Counter incremented to: ");
            Serial.println(system.getCounter());
        }
    }

    if (system.isDisplayCounter() && !system.isPendingCounterIncrement()) {
        system.updateDisplay();
    }

    if (system.getCounter() >= MAX_COUNTER && !deviceButton.isPressed()) {
        system.setState(SystemState::ERROR);
    }
}