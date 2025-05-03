#include "SetupStateHandler.h"

void SetupStateHandler::handle(DeviceSystem& system) {
    if (system.getDeviceButton().wasJustPressed()) {
        system.getSetupCounter()++;
        if (system.getSetupCounter() >= MAX_COUNTER) {
            system.getSetupCounter() = 0;
        }
        system.getLeds().updateSetupDisplay(system.getSetupCounter());
        Serial.print("Setup: LEDs turned off = ");
        Serial.println(system.getSetupCounter());
    }

    static bool gameStarted = false;
    if (!gameStarted && system.getActionButton().wasHeldFor(1000)) {
        gameStarted = true;
        system.getCounter() = system.getSetupCounter();
        system.isDisplayCounter() = true;
        system.getLeds().updateCounterLeds(system.getCounter());
        system.setState(SystemState::NORMAL);
        system.getSound().playConfirmationBeep();
        Serial.println("Setup complete. Starting game...");
    }
}