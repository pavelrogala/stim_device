#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <Config.h>

class LedManager {
private:
    const int* counterLedPins;
    const int ledCount;
    int lastCounter = -1;

public:
    LedManager(const int* pins, int count);
    void turnOffCounterLeds();
    void updateCounterLeds(int counter);
    void animateCounterLeds(int counter);
    void setActionStarted(bool on);
    void setActionCompleted(bool on);
    void updateSetupDisplay(int setupCounter);
};

#endif