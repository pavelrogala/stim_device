#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include <Config.h>

class SoundManager {
private:
    const int pin;

public:
    SoundManager(int pin);
    void playConfirmationBeep();
    void playErrorTone();
    void playSweepTone(float progress);
    void stopTone();
};

#endif