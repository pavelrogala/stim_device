#include <Arduino.h>

// === Constants ===
constexpr int DEVICE_BUTTON_PIN = 2;
constexpr int ACTION_BUTTON_PIN = 10;
constexpr int LED_ACTION_STARTED_PIN = 13;
constexpr int LED_ACTION_COMPLETED_PIN = 8;
constexpr int BUZZER_PIN = 9;
constexpr int COUNTER_LED_PINS[5] = {3, 4, 5, 6, 7};

constexpr int MAX_COUNTER = 5;
constexpr unsigned long ACTION_HOLD_TIME_MS = 2000;
constexpr unsigned long DISPLAY_TIMEOUT_MS = 5000;
constexpr int BEEP_FREQUENCY = 700;
constexpr int ERROR_FREQUENCY = 300;
constexpr int LED_ANIMATION_SPEED = 150;

// Animation timing constants
constexpr unsigned long BEEP_DURATION = 100;
constexpr unsigned long ERROR_TONE_DURATION = 150;
constexpr unsigned long LED_FLICKER_MIN = 30;
constexpr unsigned long LED_FLICKER_MAX = 120;

enum class SystemState {
    SETUP,
    NORMAL,
    ERROR
};

// === Animation State ===
struct AnimationState {
    unsigned long lastUpdateTime = 0;
    int currentStep = 0;
    bool isActive = false;
    int targetLed = -1;
    int flickerCount = 0;
    bool ledState = false;
};

class Button {
private:
    const int pin;
    bool previouslyPressed = false;

public:
    Button(int pin) : pin(pin) {
        pinMode(pin, INPUT_PULLUP);
    }

    bool isPressed() const {
        return digitalRead(pin) == LOW;
    }

    bool wasJustPressed() {
        bool currentState = isPressed();
        bool result = currentState && !previouslyPressed;
        previouslyPressed = currentState;
        return result;
    }

    bool wasJustReleased() {
        bool currentState = isPressed();
        bool result = !currentState && previouslyPressed;
        previouslyPressed = currentState;
        return result;
    }
};

class LedManager {
private:
    const int* counterLedPins;
    const int ledCount;
    int lastCounter = -1;
    AnimationState animationState;

public:
    LedManager(const int* pins, int count)
        : counterLedPins(pins), ledCount(count) {
        for (int i = 0; i < count; i++) {
            pinMode(counterLedPins[i], OUTPUT);
            digitalWrite(counterLedPins[i], LOW);
        }
        pinMode(LED_ACTION_STARTED_PIN, OUTPUT);
        pinMode(LED_ACTION_COMPLETED_PIN, OUTPUT);
    }

    void turnOffCounterLeds() {
        for (int i = 0; i < ledCount; i++) {
            digitalWrite(counterLedPins[i], LOW);
        }
        lastCounter = -1;
        animationState.isActive = false;
    }

    void updateCounterLeds(int counter) {
        int ledsOn = MAX_COUNTER - counter;

        if (lastCounter != -1 && counter > lastCounter) {
            startFlickerAnimation(MAX_COUNTER - lastCounter - 1);
        }

        if (!animationState.isActive) {
            for (int i = 0; i < ledCount; i++) {
                if (i < ledsOn) {
                    digitalWrite(counterLedPins[i], HIGH);
                } else {
                    digitalWrite(counterLedPins[i], LOW);
                }
            }
        }

        lastCounter = counter;
    }

    void startFlickerAnimation(int ledIndex) {
        if (ledIndex >= 0 && ledIndex < ledCount) {
            animationState.isActive = true;
            animationState.targetLed = ledIndex;
            animationState.flickerCount = random(3, 6);
            animationState.currentStep = 0;
            animationState.lastUpdateTime = millis();
            animationState.ledState = false;
        }
    }

    void updateAnimation() {
        if (!animationState.isActive) return;

        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - animationState.lastUpdateTime;

        if (elapsedTime >= (animationState.ledState ? LED_FLICKER_MAX : LED_FLICKER_MIN)) {
            animationState.ledState = !animationState.ledState;
            digitalWrite(counterLedPins[animationState.targetLed], animationState.ledState ? HIGH : LOW);
            animationState.lastUpdateTime = currentTime;

            if (!animationState.ledState) {
                animationState.currentStep++;
                if (animationState.currentStep >= animationState.flickerCount) {
                    animationState.isActive = false;
                    digitalWrite(counterLedPins[animationState.targetLed], LOW);
                }
            }
        }
    }

    void animateCounterLeds(int counter) {
        int ledsOn = MAX_COUNTER - counter;
        for (int i = 0; i < ledsOn; i++) {
            digitalWrite(counterLedPins[i], HIGH);
            delay(LED_ANIMATION_SPEED);
        }
        for (int i = ledsOn; i < ledCount; i++) {
            digitalWrite(counterLedPins[i], LOW);
        }
    }

    void setActionStarted(bool on) {
        digitalWrite(LED_ACTION_STARTED_PIN, on ? HIGH : LOW);
    }

    void setActionCompleted(bool on) {
        digitalWrite(LED_ACTION_COMPLETED_PIN, on ? HIGH : LOW);
    }

    void updateSetupDisplay(int setupCounter) {
        int ledsOn = MAX_COUNTER - setupCounter;
        for (int i = 0; i < MAX_COUNTER; i++) {
            if (i < ledsOn) {
                digitalWrite(counterLedPins[i], HIGH);
            } else {
                digitalWrite(counterLedPins[i], LOW);
            }
        }
    }
};

class SoundManager {
private:
    const int pin;
    unsigned long lastBeepTime = 0;
    int beepCount = 0;
    bool isBeeping = false;
    bool isErrorTone = false;
    bool isSweeping = false;
    unsigned long sweepStartTime = 0;
    float sweepProgress = 0.0f;

public:
    SoundManager(int pin) : pin(pin) {
        pinMode(pin, OUTPUT);
    }

    void playConfirmationBeep() {
        beepCount = 3;
        isBeeping = true;
        lastBeepTime = millis();
        tone(pin, BEEP_FREQUENCY);
    }

    void playErrorTone() {
        isErrorTone = true;
        lastBeepTime = millis();
        tone(pin, BEEP_FREQUENCY);
    }

    void playSweepTone(float progress) {
        if (!isSweeping) {
            isSweeping = true;
            sweepStartTime = millis();
        }
        sweepProgress = progress;
        int freq = ERROR_FREQUENCY + (progress * (BEEP_FREQUENCY - ERROR_FREQUENCY));
        tone(pin, freq);
    }

    void stopTone() {
        noTone(pin);
        isBeeping = false;
        isErrorTone = false;
        isSweeping = false;
        beepCount = 0;
    }

    void update() {
        unsigned long currentTime = millis();

        if (isBeeping) {
            if (currentTime - lastBeepTime >= BEEP_DURATION) {
                if (digitalRead(pin) == HIGH) {
                    noTone(pin);
                    lastBeepTime = currentTime;
                } else {
                    if (beepCount > 0) {
                        tone(pin, BEEP_FREQUENCY);
                        beepCount--;
                    } else {
                        isBeeping = false;
                    }
                }
            }
        }

        if (isErrorTone) {
            if (currentTime - lastBeepTime >= ERROR_TONE_DURATION) {
                if (digitalRead(pin) == HIGH) {
                    tone(pin, ERROR_FREQUENCY);
                } else {
                    isErrorTone = false;
                    noTone(pin);
                }
                lastBeepTime = currentTime;
            }
        }
    }
};

class DeviceSystem {
private:
    Button deviceButton;
    Button actionButton;
    LedManager leds;
    SoundManager sound;
    SystemState state = SystemState::SETUP;
    int counter = 0;
    int setupCounter = 0;
    bool pendingCounterIncrement = false;
    bool actionInProgress = false;
    bool actionDoneThisCycle = false;
    bool displayCounter = false;
    bool deviceButtonCurrentlyPressed = false;
    unsigned long actionStartTime = 0;
    unsigned long deviceButtonReleaseTime = 0;

public:
    DeviceSystem()
        : deviceButton(DEVICE_BUTTON_PIN)
        , actionButton(ACTION_BUTTON_PIN)
        , leds(COUNTER_LED_PINS, MAX_COUNTER)
        , sound(BUZZER_PIN) {}

    void update() {
        if (state == SystemState::ERROR) {
            handleErrorState();
        } else if (state == SystemState::SETUP) {
            handleSetupState();
        } else {
            handleNormalState();
        }

        // Update animations and sounds
        leds.updateAnimation();
        sound.update();
    }

    void begin() {
        leds.updateSetupDisplay(setupCounter);
    }

private:
    void handleErrorState() {
        leds.turnOffCounterLeds();
        leds.setActionStarted(false);
        leds.setActionCompleted(false);
        sound.stopTone();

        if (deviceButton.wasJustPressed()) {
            sound.playErrorTone();

            for (int i = 0; i < 3; i++) {
                digitalWrite(LED_ACTION_STARTED_PIN, HIGH);
                delay(150);
                digitalWrite(LED_ACTION_STARTED_PIN, LOW);
                delay(150);
            }
        }
    }

    void handleSetupState() {
        if (deviceButton.wasJustPressed()) {
            if (setupCounter < MAX_COUNTER - 1) {
                setupCounter++;
                leds.updateSetupDisplay(setupCounter);
                Serial.print("Setup: LEDs turned off = ");
                Serial.println(setupCounter);
            } else {
                sound.playErrorTone();
                Serial.println("Setup: Cannot have 0 lives!");
            }
        }

        if (actionButton.wasJustPressed()) {
            counter = setupCounter;
            displayCounter = true;
            leds.updateCounterLeds(counter);
            state = SystemState::NORMAL;
            sound.playConfirmationBeep();
            Serial.println("Setup complete. Starting game...");
        }
    }

    void handleNormalState() {
        bool devicePressed = deviceButton.isPressed();
        bool deviceJustPressed = deviceButton.wasJustPressed();
        bool deviceJustReleased = deviceButton.wasJustReleased();
        deviceButtonCurrentlyPressed = devicePressed;

        if (devicePressed) {
            deviceButtonReleaseTime = millis();
            
            if (deviceJustPressed) {
                displayCounter = true;
                playCounterAnimation();
            }

            if (!actionDoneThisCycle && actionButton.wasJustPressed()) {
                startAction();
            }
        } else {
            if (millis() - deviceButtonReleaseTime >= DISPLAY_TIMEOUT_MS) {
                displayCounter = false;
                leds.turnOffCounterLeds();
            }
            if (actionInProgress) {
                cancelAction();
            }
            actionDoneThisCycle = false;
        }

        if (actionInProgress) {
            handleActionInProgress();
        }

        if (actionButton.wasJustReleased()) {
            leds.setActionCompleted(false);
            sound.stopTone();
            actionInProgress = false;

            if (pendingCounterIncrement && counter < MAX_COUNTER) {
                counter++;
                pendingCounterIncrement = false;
                updateDisplay();
                Serial.print("Counter incremented to: ");
                Serial.println(counter);
            }
        }

        if (displayCounter && !pendingCounterIncrement) {
            updateDisplay();
        }

        if (counter >= MAX_COUNTER && !deviceButton.isPressed()) {
            enterErrorState();
        }
    }

    void startAction() {
        actionInProgress = true;
        actionStartTime = millis();
        leds.setActionStarted(true);
    }

    void handleActionInProgress() {
        unsigned long heldDuration = millis() - actionStartTime;
        float progress = min((float)heldDuration / ACTION_HOLD_TIME_MS, 1.0f);
        sound.playSweepTone(progress);

        if (!actionButton.isPressed()) {
            cancelAction();
        }

        if (heldDuration >= ACTION_HOLD_TIME_MS && !actionDoneThisCycle) {
            completeAction();
        }
    }

    void completeAction() {
        actionDoneThisCycle = true;
        pendingCounterIncrement = true;
        actionInProgress = false;

        leds.setActionStarted(false);
        leds.setActionCompleted(true);
        sound.stopTone();
        sound.playConfirmationBeep();
        Serial.println("Action completed");
    }

    void cancelAction() {
        actionInProgress = false;
        leds.setActionStarted(false);
        leds.setActionCompleted(false);
        sound.stopTone();
        pendingCounterIncrement = false;
        Serial.println("Action cancelled");
    }

    void playCounterAnimation() {
        leds.animateCounterLeds(counter);
    }

    void updateDisplay() {
        if (displayCounter) {
            leds.updateCounterLeds(counter);
        } else {
            leds.turnOffCounterLeds();
        }
    }

    void enterErrorState() {
        state = SystemState::ERROR;
        Serial.println("Entering Error State!");
        leds.turnOffCounterLeds();
        leds.setActionStarted(false);
        leds.setActionCompleted(false);
        sound.stopTone();
    }
};

DeviceSystem deviceSystem;

void setup() {
    Serial.begin(9600);
    randomSeed(analogRead(0));  // Seed random number generator
    deviceSystem.begin();
}

void loop() {
    deviceSystem.update();
}
