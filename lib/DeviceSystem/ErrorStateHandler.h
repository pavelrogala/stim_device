#ifndef ERROR_STATE_HANDLER_H
#define ERROR_STATE_HANDLER_H

#include "SystemStateHandler.h"

class ErrorStateHandler : public SystemStateHandler {
public:
    void handle(DeviceSystem& system) override;
};

#endif // ERROR_STATE_HANDLER_H