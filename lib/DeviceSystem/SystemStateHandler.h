#ifndef SYSTEM_STATE_HANDLER_H
#define SYSTEM_STATE_HANDLER_H

#include "DeviceSystem.h"

class SystemStateHandler {
public:
    virtual ~SystemStateHandler() = default;
    virtual void handle(DeviceSystem& system) = 0;
};

#endif // SYSTEM_STATE_HANDLER_H