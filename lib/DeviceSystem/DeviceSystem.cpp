#include "DeviceSystem.h"

DeviceSystem::DeviceSystem()
    : deviceButton(DEVICE_BUTTON_PIN),
      actionButton(ACTION_BUTTON_PIN),
      leds(COUNTER_LED_PINS, MAX_COUNTER),
      sound(BUZZER_PIN),
      state(SystemState::SETUP),
      counter(0),
      setupCounter(0),
      pendingCounterIncrement(false),
      actionInProgress(false),
      actionDoneThisCycle(false),
      displayCounter(false),
      deviceButtonCurrentlyPressed(false),
      actionStartTime(0),
      deviceButtonReleaseTime(0) {}

void DeviceSystem::begin() {
    leds.updateSetupDisplay(setupCounter);
}

void DeviceSystem::update() {
    if (state == SystemState::ERROR) {
        handleErrorState();
    } else if (state == SystemState::SETUP) {
        handleSetupState();
    } else {
        handleNormalState();
    }
}

void DeviceSystem::handleErrorState() {
    leds.turnOffCounterLeds();
    leds.setActionStarted(false);
    leds.setActionCompleted(false);
    sound.stopTone();

    if (deviceButton.wasJustPressed()) {
        sound.playErrorTone();
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_ACTION_STARTED_PIN, HIGH);
            delay(ERROR_LED_FLASH_DURATION);
            digitalWrite(LED_ACTION_STARTED_PIN, LOW);
            delay(ERROR_LED_FLASH_DURATION);
        }
    }
}

void DeviceSystem::handleSetupState() {
    if (deviceButton.wasJustPressed()) {
        setupCounter++;
        if (setupCounter >= MAX_COUNTER) {
            setupCounter = 0;
        }
        leds.updateSetupDisplay(setupCounter);
        Serial.print("Setup: LEDs turned off = ");
        Serial.println(setupCounter);
    }

    static bool gameStarted = false;
    if (!gameStarted && actionButton.wasHeldFor(1000)) {
        gameStarted = true;
        counter = setupCounter;
        displayCounter = true;
        leds.updateCounterLeds(counter);
        state = SystemState::NORMAL;
        sound.playConfirmationBeep();
        Serial.println("Setup complete. Starting game...");
    }
}

void DeviceSystem::handleNormalState() {
    bool devicePressed = deviceButton.isPressed();
    bool deviceJustPressed = deviceButton.wasJustPressed();
    deviceButtonCurrentlyPressed = devicePressed;

    if (devicePressed) {
        deviceButtonReleaseTime = millis();

        if (deviceJustPressed) {
            displayCounter = true;
            playCounterAnimation();
        }

        if (!actionDoneThisCycle && actionButton.wasJustPressed()) {
            startAction();
        }
    } else {
        if (millis() - deviceButtonReleaseTime >= DISPLAY_TIMEOUT_MS) {
            displayCounter = false;
            leds.turnOffCounterLeds();
        }
        if (actionInProgress) {
            cancelAction();
        }
        actionDoneThisCycle = false;
    }

    if (actionInProgress) {
        handleActionInProgress();
    }

    if (actionButton.wasJustReleased()) {
        leds.setActionCompleted(false);
        sound.stopTone();
        actionInProgress = false;

        if (pendingCounterIncrement && counter < MAX_COUNTER) {
            counter++;
            pendingCounterIncrement = false;
            updateDisplay();
            Serial.print("Counter incremented to: ");
            Serial.println(counter);
        }
    }

    if (displayCounter && !pendingCounterIncrement) {
        updateDisplay();
    }

    if (counter >= MAX_COUNTER && !deviceButton.isPressed()) {
        enterErrorState();
    }
}

void DeviceSystem::startAction() {
    actionInProgress = true;
    actionStartTime = millis();
    leds.setActionStarted(true);
}

void DeviceSystem::handleActionInProgress() {
    unsigned long heldDuration = millis() - actionStartTime;
    float progress = min((float)heldDuration / ACTION_HOLD_TIME_MS, 1.0f);
    sound.playSweepTone(progress);

    if (!actionButton.isPressed()) {
        cancelAction();
    }

    if (heldDuration >= ACTION_HOLD_TIME_MS && !actionDoneThisCycle) {
        completeAction();
    }
}

void DeviceSystem::completeAction() {
    actionDoneThisCycle = true;
    pendingCounterIncrement = true;
    actionInProgress = false;

    leds.setActionStarted(false);
    leds.setActionCompleted(true);
    sound.stopTone();
    sound.playConfirmationBeep();
    Serial.println("Action completed");
}

void DeviceSystem::cancelAction() {
    actionInProgress = false;
    leds.setActionStarted(false);
    leds.setActionCompleted(false);
    sound.stopTone();
    pendingCounterIncrement = false;
    Serial.println("Action cancelled");
}

void DeviceSystem::playCounterAnimation() {
    leds.animateCounterLeds(counter);
}

void DeviceSystem::updateDisplay() {
    if (displayCounter) {
        leds.updateCounterLeds(counter);
    } else {
        leds.turnOffCounterLeds();
    }
}

void DeviceSystem::enterErrorState() {
    state = SystemState::ERROR;
    Serial.println("Entering Error State!");
    leds.turnOffCounterLeds();
    leds.setActionStarted(false);
    leds.setActionCompleted(false);
    sound.stopTone();
}