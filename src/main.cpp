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

enum class SystemState {
    NORMAL,
    ERROR
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
    }

    void updateCounterLeds(int counter) {
        int ledsOn = MAX_COUNTER - counter;
        for (int i = 0; i < ledCount; i++) {
            digitalWrite(counterLedPins[i], (i < ledsOn) ? HIGH : LOW);
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
};

class SoundManager {
private:
    const int pin;

public:
    SoundManager(int pin) : pin(pin) {
        pinMode(pin, OUTPUT);
    }

    void playConfirmationBeep() {
        for (int i = 0; i < 3; i++) {
            tone(pin, BEEP_FREQUENCY);
            delay(100);
            noTone(pin);
            delay(100);
        }
    }

    void playErrorTone() {
        tone(pin, BEEP_FREQUENCY);
        delay(150);
        tone(pin, ERROR_FREQUENCY);
        delay(150);
        noTone(pin);
    }

    void playSweepTone(float progress) {
        int freq = ERROR_FREQUENCY + (progress * (BEEP_FREQUENCY - ERROR_FREQUENCY));
        tone(pin, freq);
    }

    void stopTone() {
        noTone(pin);
    }
};

class DeviceSystem {
private:
    Button deviceButton;
    Button actionButton;
    LedManager leds;
    SoundManager sound;
    SystemState state = SystemState::NORMAL;
    int counter = 0;
    bool pendingCounterIncrement = false;
    bool actionInProgress = false;
    bool actionDoneThisCycle = false;
    bool displayCounter = false;
    bool animationPlayed = false;
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
        } else {
            handleNormalState();
        }
    }

private:
    void handleErrorState() {
        leds.turnOffCounterLeds();
        leds.setActionStarted(false);
        leds.setActionCompleted(false);
        sound.stopTone();

        if (deviceButton.wasJustPressed()) {
            sound.playErrorTone();
        }
    }

    void handleNormalState() {
        bool devicePressed = deviceButton.isPressed();
        deviceButtonCurrentlyPressed = devicePressed;

        if (devicePressed) {
            deviceButtonReleaseTime = millis();
            displayCounter = true;

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

    void updateDisplay() {
        if (displayCounter) {
            if (!animationPlayed) {
                leds.animateCounterLeds(counter);
                animationPlayed = true;
            }
        } else {
            leds.turnOffCounterLeds();
            animationPlayed = false;
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
}

void loop() {
    deviceSystem.update();
}
