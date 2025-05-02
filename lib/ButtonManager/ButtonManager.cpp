#include "ButtonManager.h"

Button::Button(int pin) : pin(pin) {
    pinMode(pin, INPUT_PULLUP);
}

bool Button::isPressed() {
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

bool Button::wasJustPressed() {
    bool pressed = isPressed();
    bool result = pressed && !previouslyPressed;
    if (result) {
        pressStartTime = millis();
    }
    previouslyPressed = pressed;
    return result;
}

bool Button::wasJustReleased() {
    bool pressed = isPressed();
    bool result = !pressed && previouslyPressed;
    previouslyPressed = pressed;
    return result;
}

bool Button::wasHeldFor(unsigned long duration) {
    if (isPressed()) {
        if (!previouslyPressed) {
            pressStartTime = millis();
        }
        previouslyPressed = true;
        if ((millis() - pressStartTime >= duration) && !alreadyReportedHold) {
            alreadyReportedHold = true;
            return true;
        }
    } else {
        previouslyPressed = false;
        alreadyReportedHold = false;
    }
    return false;
}
