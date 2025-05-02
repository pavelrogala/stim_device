#include "SoundManager.h"
#include <Config.h>  // Needed for the beep constants

SoundManager::SoundManager(int pin) : pin(pin) {
    pinMode(pin, OUTPUT);
}

void SoundManager::playConfirmationBeep() {
    for (int i = 0; i < CONFIRMATION_BEEPS; i++) {
        tone(pin, BEEP_FREQUENCY);
        delay(BEEP_DURATION);
        noTone(pin);
        delay(BEEP_PAUSE);
    }
}

void SoundManager::playErrorTone() {
    tone(pin, BEEP_FREQUENCY);
    delay(ERROR_TONE_DURATION);
    tone(pin, ERROR_FREQUENCY);
    delay(ERROR_TONE_DURATION);
    noTone(pin);
}

void SoundManager::playSweepTone(float progress) {
    int freq = ERROR_FREQUENCY + (progress * (BEEP_FREQUENCY - ERROR_FREQUENCY));
    tone(pin, freq);
}

void SoundManager::stopTone() {
    noTone(pin);
}
