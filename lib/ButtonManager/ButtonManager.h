#pragma once
#include <Arduino.h>

class ButtonManager {
private:
    const int pin;
    bool previouslyPressed = false;
    unsigned long lastDebounceTime = 0;
    bool lastStableState = HIGH;
    bool lastReadState = HIGH;
    static constexpr unsigned long debounceDelay = 50;
    unsigned long pressStartTime = 0;
public:
    ButtonManager(int pin);

    bool isPressed();
    bool wasJustPressed();
    bool wasJustReleased();
    bool wasHeldFor(unsigned long durationMs);
};
