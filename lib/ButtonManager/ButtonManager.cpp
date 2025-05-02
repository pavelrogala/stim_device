#include "ButtonManager.h"

ButtonManager::ButtonManager(int pin) : pin(pin) {
    pinMode(pin, INPUT_PULLUP);
}

bool ButtonManager::isPressed() {
    bool reading = digitalRead(pin);
    if (reading != lastReadState) {
        lastDebounceTime = millis();
        lastReadState = reading;
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (lastStableState != reading) {
            lastStableState = reading;
        }
    }
    return lastStableState == LOW;
}

bool ButtonManager::wasJustPressed() {
    bool pressed = isPressed();
    bool result = pressed && !previouslyPressed;
    previouslyPressed = pressed;
    return result;
}

bool ButtonManager::wasJustReleased() {
    bool pressed = isPressed();
    bool result = !pressed && previouslyPressed;
    previouslyPressed = pressed;
    return result;
}

bool ButtonManager::wasHeldFor(unsigned long durationMs) {
    bool pressed = isPressed();

    if (pressed && !previouslyPressed) {
        // Just started holding
        pressStartTime = millis();
    }

    previouslyPressed = pressed;

    return pressed && (millis() - pressStartTime >= durationMs);
}

