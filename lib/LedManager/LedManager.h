#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include <Arduino.h>

class LedManager {
private:
    const int* counterLedPins;
    const int ledCount;
    int lastCounter;

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
