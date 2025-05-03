#ifndef NORMAL_STATE_HANDLER_H
#define NORMAL_STATE_HANDLER_H

#include "SystemStateHandler.h"

class NormalStateHandler : public SystemStateHandler {
public:
    void handle(DeviceSystem& system) override;
};

#endif // NORMAL_STATE_HANDLER_H