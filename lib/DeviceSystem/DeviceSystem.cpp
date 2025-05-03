#include "DeviceSystem.h"
#include "SystemStateHandler.h"
#include "ErrorStateHandler.h"
#include "SetupStateHandler.h"
#include "NormalStateHandler.h"

DeviceSystem::DeviceSystem()
    : deviceButton(DEVICE_BUTTON_PIN),
      actionButton(ACTION_BUTTON_PIN),
      leds(COUNTER_LED_PINS, MAX_COUNTER),
      sound(BUZZER_PIN),
      state(SystemState::SETUP),
      currentStateHandler(new SetupStateHandler()),
      counter(0),
      setupCounter(0),
      pendingCounterIncrement(false),
      actionInProgress(false),
      actionDoneThisCycle(false),
      displayCounter(false),
      deviceButtonCurrentlyPressed(false),
      actionStartTime(0),
      deviceButtonReleaseTime(0) {}

DeviceSystem::~DeviceSystem() {
    delete currentStateHandler;
}

void DeviceSystem::begin() {
    leds.updateSetupDisplay(setupCounter);
}

void DeviceSystem::update() {
    if (currentStateHandler) {
        currentStateHandler->handle(*this);
    }
}

void DeviceSystem::setState(SystemState newState) {
    delete currentStateHandler;
    state = newState;

    switch (newState) {
        case SystemState::SETUP:
            currentStateHandler = new SetupStateHandler();
            break;
        case SystemState::NORMAL:
            currentStateHandler = new NormalStateHandler();
            break;
        case SystemState::ERROR:
            currentStateHandler = new ErrorStateHandler();
            break;
    }
}

void DeviceSystem::resetLeds() {
    leds.turnOffCounterLeds();
    leds.setActionStarted(false);
    leds.setActionCompleted(false);
}

void DeviceSystem::playErrorSequence() {
    sound.playErrorTone();
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_ACTION_STARTED_PIN, HIGH);
        delay(ERROR_LED_FLASH_DURATION);
        digitalWrite(LED_ACTION_STARTED_PIN, LOW);
        delay(ERROR_LED_FLASH_DURATION);
    }
}

void DeviceSystem::updateDisplay() {
    if (displayCounter) {
        leds.updateCounterLeds(counter);
    } else {
        leds.turnOffCounterLeds();
    }
}

// Getters for state handlers
ButtonManager& DeviceSystem::getDeviceButton() { return deviceButton; }
ButtonManager& DeviceSystem::getActionButton() { return actionButton; }
LedManager& DeviceSystem::getLeds() { return leds; }
SoundManager& DeviceSystem::getSound() { return sound; }
int& DeviceSystem::getCounter() { return counter; }
int& DeviceSystem::getSetupCounter() { return setupCounter; }
bool& DeviceSystem::isPendingCounterIncrement() { return pendingCounterIncrement; }
bool& DeviceSystem::isActionInProgress() { return actionInProgress; }
bool& DeviceSystem::isActionDoneThisCycle() { return actionDoneThisCycle; }
bool& DeviceSystem::isDisplayCounter() { return displayCounter; }
bool& DeviceSystem::isDeviceButtonCurrentlyPressed() { return deviceButtonCurrentlyPressed; }
unsigned long& DeviceSystem::getActionStartTime() { return actionStartTime; }
unsigned long& DeviceSystem::getDeviceButtonReleaseTime() { return deviceButtonReleaseTime; }