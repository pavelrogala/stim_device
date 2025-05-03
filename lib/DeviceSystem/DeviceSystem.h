#ifndef DEVICE_SYSTEM_H
#define DEVICE_SYSTEM_H

#include <Arduino.h>
#include "ButtonManager.h"
#include "LedManager.h"
#include "SoundManager.h"
#include "Config.h"

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
    void begin();
    void update();

private:
    void handleErrorState();
    void handleSetupState();
    void handleNormalState();
    void startAction();
    void handleActionInProgress();
    void completeAction();
    void cancelAction();
    void playCounterAnimation();
    void updateDisplay();
    void enterErrorState();
};

#endif // DEVICE_SYSTEM_H