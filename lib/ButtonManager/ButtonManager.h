#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include <Config.h>

class Button {
private:
    const int pin;
    bool previouslyPressed = false;
    bool lastStableState = HIGH;
    bool lastReadState = HIGH;
    unsigned long lastDebounceTime = 0;
    unsigned long pressStartTime = 0;
    static constexpr unsigned long debounceDelay = 50;
    bool alreadyReportedHold = false;
public:
    Button(int pin);
    bool isPressed();
    bool wasJustPressed();
    bool wasJustReleased();
    bool wasHeldFor(unsigned long duration);
};

#endif