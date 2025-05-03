#include "ErrorStateHandler.h"

void ErrorStateHandler::handle(DeviceSystem& system) {
    system.resetLeds();
    if (system.getDeviceButton().wasJustPressed()) {
        system.playErrorSequence();
    }
}