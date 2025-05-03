#ifndef DEVICE_SYSTEM_H
#define DEVICE_SYSTEM_H

#include <Arduino.h>
#include "ButtonManager.h"
#include "LedManager.h"
#include "SoundManager.h"
#include "Config.h"

class SystemStateHandler; // Forward declaration

// === SYSTEM STATES ===
enum class SystemState {
    SETUP,
    NORMAL,
    ERROR
};

class DeviceSystem {
private:
    ButtonManager deviceButton;
    ButtonManager actionButton;
    LedManager leds;
    SoundManager sound;
    SystemState state;
    SystemStateHandler* currentStateHandler; // Pointer to current state handler

    int counter;
    int setupCounter;
    bool pendingCounterIncrement;
    bool actionInProgress;
    bool actionDoneThisCycle;
    bool displayCounter;
    bool deviceButtonCurrentlyPressed;
    unsigned long actionStartTime;
    unsigned long deviceButtonReleaseTime;

public:
    DeviceSystem();
    ~DeviceSystem();

    void begin();
    void update();

    // State transition
    void setState(SystemState newState);

    // Helper methods
    void resetLeds();
    void playErrorSequence();
    void updateDisplay();

    // Getters for state handlers
    ButtonManager& getDeviceButton();
    ButtonManager& getActionButton();
    LedManager& getLeds();
    SoundManager& getSound();
    int& getCounter();
    int& getSetupCounter();
    bool& isPendingCounterIncrement();
    bool& isActionInProgress();
    bool& isActionDoneThisCycle();
    bool& isDisplayCounter();
    bool& isDeviceButtonCurrentlyPressed();
    unsigned long& getActionStartTime();
    unsigned long& getDeviceButtonReleaseTime();
};

#endif // DEVICE_SYSTEM_H