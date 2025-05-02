#include "LedManager.h"
#include <Config.h>  // Only needed here, not in the header

LedManager::LedManager(const int* pins, int count)
    : counterLedPins(pins), ledCount(count), lastCounter(-1) {
    for (int i = 0; i < count; i++) {
        pinMode(counterLedPins[i], OUTPUT);
        digitalWrite(counterLedPins[i], LOW);
    }
    pinMode(LED_ACTION_STARTED_PIN, OUTPUT);
    pinMode(LED_ACTION_COMPLETED_PIN, OUTPUT);
}

void LedManager::turnOffCounterLeds() {
    for (int i = 0; i < ledCount; i++) {
        digitalWrite(counterLedPins[i], LOW);
    }
    lastCounter = -1;
}

void LedManager::updateCounterLeds(int counter) {
    int ledsOn = MAX_COUNTER - counter;

    if (lastCounter != -1 && counter > lastCounter) {
        int ledToFlicker = MAX_COUNTER - lastCounter - 1;
        if (ledToFlicker >= 0 && ledToFlicker < ledCount) {
            int flickers = random(LED_FLICKER_MIN_COUNT, LED_FLICKER_MAX_COUNT);
            for (int i = 0; i < flickers; i++) {
                digitalWrite(counterLedPins[ledToFlicker], LOW);
                delay(random(LED_FLICKER_MIN, LED_FLICKER_MAX));
                digitalWrite(counterLedPins[ledToFlicker], HIGH);
                delay(random(LED_FLICKER_MIN, LED_FLICKER_MAX));
            }
            digitalWrite(counterLedPins[ledToFlicker], LOW);
        }
    }

    for (int i = 0; i < ledCount; i++) {
        digitalWrite(counterLedPins[i], i < ledsOn ? HIGH : LOW);
    }

    lastCounter = counter;
}

void LedManager::animateCounterLeds(int counter) {
    int ledsOn = MAX_COUNTER - counter;
    for (int i = 0; i < ledsOn; i++) {
        digitalWrite(counterLedPins[i], HIGH);
        delay(LED_ANIMATION_SPEED);
    }
    for (int i = ledsOn; i < ledCount; i++) {
        digitalWrite(counterLedPins[i], LOW);
    }
}

void LedManager::setActionStarted(bool on) {
    digitalWrite(LED_ACTION_STARTED_PIN, on ? HIGH : LOW);
}

void LedManager::setActionCompleted(bool on) {
    digitalWrite(LED_ACTION_COMPLETED_PIN, on ? HIGH : LOW);
}

void LedManager::updateSetupDisplay(int setupCounter) {
    int ledsOn = MAX_COUNTER - setupCounter;
    for (int i = 0; i < MAX_COUNTER; i++) {
        digitalWrite(counterLedPins[i], i < ledsOn ? HIGH : LOW);
    }
}
