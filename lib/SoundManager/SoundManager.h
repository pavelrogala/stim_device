#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <Arduino.h>

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
