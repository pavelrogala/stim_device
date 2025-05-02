#include <Arduino.h>
#include "Config.h"
#include "ButtonManager.h"
#include "LedManager.h"
#include "SoundManager.h"

Button deviceButton(DEVICE_BUTTON_PIN);
Button actionButton(ACTION_BUTTON_PIN);
LedManager leds(COUNTER_LED_PINS, MAX_COUNTER);
SoundManager sound(BUZZER_PIN);
bool actionDone = false;
bool pendingIncrement = false;


enum class State { SETUP, NORMAL, ERROR };
State state = State::SETUP;

int setupCounter = 0;
int counter = 0;
bool showCounter = false;

bool actionInProgress = false;
bool actionJustCompleted = false;
unsigned long actionStart = 0;
unsigned long deviceReleaseTime = 0;

void enterErrorState() {
    state = State::ERROR;
    Serial.println("Entering ERROR state");
    leds.turnOffCounterLeds();
    leds.setActionStarted(false);
    leds.setActionCompleted(false);
    sound.stopTone();
}

void setup() {
    Serial.begin(9600);
    leds.updateSetupDisplay(setupCounter);
}

void loop() {
    switch (state) {
        case State::SETUP:
            if (deviceButton.wasJustPressed()) {
                setupCounter = (setupCounter + 1) % MAX_COUNTER;
                leds.updateSetupDisplay(setupCounter);
                Serial.print("Setup: ");
                Serial.println(setupCounter);
            }

            if (actionButton.wasHeldFor(1000)) {
                counter = setupCounter;
                leds.updateCounterLeds(counter);
                showCounter = true;
                state = State::NORMAL;
                sound.playConfirmationBeep();
                Serial.println("Setup done. Entering NORMAL state.");
            }
            break;

            case State::NORMAL: {
                bool devicePressed = deviceButton.isPressed();
                bool deviceJustPressed = deviceButton.wasJustPressed();
                bool actionPressed = actionButton.isPressed();
                bool actionJustPressed = actionButton.wasJustPressed();
                bool actionJustReleased = actionButton.wasJustReleased();
            
                // --- Device button handling ---
                if (devicePressed) {
                    deviceReleaseTime = millis();
            
                    if (deviceJustPressed) {
                        showCounter = true;
                        leds.animateCounterLeds(counter);
                    }
            
                    // --- Start action if not done yet ---
                    if (!actionDone && actionJustPressed) {
                        actionInProgress = true;
                        actionStart = millis();
                        leds.setActionStarted(true);
                    }
                } else {
                    // --- Device released: turn off counter LEDs after timeout ---
                    if (millis() - deviceReleaseTime >= DISPLAY_TIMEOUT_MS) {
                        showCounter = false;
                        leds.turnOffCounterLeds();
                    }
            
                    // Cancel action if device released
                    if (actionInProgress) {
                        actionInProgress = false;
                        leds.setActionStarted(false);
                        leds.setActionCompleted(false);
                        sound.stopTone();
                        Serial.println("Action cancelled");
                    }
            
                    // Reset done flag so a new hold is allowed
                    actionDone = false;
                }
            
                // --- While action is in progress ---
                if (actionInProgress) {
                    unsigned long held = millis() - actionStart;
                    float progress = min((float)held / ACTION_HOLD_TIME_MS, 1.0f);
                    sound.playSweepTone(progress);
            
                    if (!actionPressed) {
                        // Cancel if released early
                        actionInProgress = false;
                        leds.setActionStarted(false);
                        leds.setActionCompleted(false);
                        sound.stopTone();
                        Serial.println("Action cancelled early");
                    } else if (held >= ACTION_HOLD_TIME_MS && !actionDone) {
                        // Complete action
                        actionDone = true;
                        pendingIncrement = true;
                        actionInProgress = false;
                        leds.setActionStarted(false);
                        leds.setActionCompleted(true);
                        sound.stopTone();
                        sound.playConfirmationBeep();
                        Serial.println("Action completed");
                    }
                }
            
                // --- Finalize action on release ---
                if (actionJustReleased) {
                    leds.setActionCompleted(false);
                    sound.stopTone();
            
                    if (pendingIncrement && counter < MAX_COUNTER) {
                        counter++;
                        pendingIncrement = false;
                        leds.updateCounterLeds(counter);
                        Serial.print("Counter: ");
                        Serial.println(counter);
                    }
                }
            
                // --- Keep counter LEDs updated if showing ---
                if (showCounter && !pendingIncrement) {
                    leds.updateCounterLeds(counter);
                }
            
                // --- Error state if limit reached ---
                if (counter >= MAX_COUNTER && !devicePressed) {
                    enterErrorState();
                }
            
                break;
            }
                     
            

        case State::ERROR:
            leds.turnOffCounterLeds();
            leds.setActionStarted(false);
            leds.setActionCompleted(false);
            sound.stopTone();

            if (deviceButton.wasJustPressed()) {
                sound.playErrorTone();
                for (int i = 0; i < 3; i++) {
                    leds.setActionStarted(true);
                    delay(ERROR_LED_FLASH_DURATION);
                    leds.setActionStarted(false);
                    delay(ERROR_LED_FLASH_DURATION);
                }
            }
            break;
    }
}
