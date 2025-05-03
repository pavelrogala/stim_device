#ifndef SETUP_STATE_HANDLER_H
#define SETUP_STATE_HANDLER_H

#include "SystemStateHandler.h"

class SetupStateHandler : public SystemStateHandler {
public:
    void handle(DeviceSystem& system) override;
};

#endif // SETUP_STATE_HANDLER_H