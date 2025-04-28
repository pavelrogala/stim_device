#include <Arduino.h>

// === Constants ===
// These are the pin numbers for each component. If you change the wiring, update these numbers.
constexpr int DEVICE_BUTTON_PIN = 2;    // The main button (D2) that activates the device
constexpr int ACTION_BUTTON_PIN = 10;   // The action button (D10) that triggers the counter
constexpr int LED_ACTION_STARTED_PIN = 13;  // LED that shows when action is in progress
constexpr int LED_ACTION_COMPLETED_PIN = 8; // LED that shows when action is completed
constexpr int BUZZER_PIN = 9;           // The buzzer pin for sound effects
constexpr int COUNTER_LED_PINS[5] = {3, 4, 5, 6, 7};  // The 5 LEDs that show the counter

// === Configuration Parameters ===
// These values can be changed to modify the device's behavior
constexpr int MAX_COUNTER = 5;          // Maximum number of actions before error state
constexpr unsigned long ACTION_HOLD_TIME_MS = 2000;  // How long to hold D10 for action complete (in milliseconds)
constexpr unsigned long DISPLAY_TIMEOUT_MS = 5000;   // How long to show counter after releasing D2 (in milliseconds)
constexpr int BEEP_FREQUENCY = 700;     // Frequency of the confirmation beep (in Hz)
constexpr int ERROR_FREQUENCY = 300;    // Frequency of the error tone (in Hz)

// === System State ===
// The device can be in one of two states: normal operation or error state
enum class SystemState {
    NORMAL,  // Normal operation mode
    ERROR    // Error state (when counter reaches maximum)
};

// === Button Class ===
// Handles button input and detects when buttons are pressed or released
class Button {
private:
    const int pin;  // The pin number this button is connected to
    bool previouslyPressed = false;  // Tracks the previous state of the button

public:
    Button(int pin) : pin(pin) {
        pinMode(pin, INPUT_PULLUP);  // Set up the button pin with internal pull-up resistor
    }

    bool isPressed() const {
        return digitalRead(pin) == LOW;  // Button is pressed when pin reads LOW
    }

    bool wasJustPressed() {
        bool currentState = isPressed();
        bool result = currentState && !previouslyPressed;  // True only on the first frame the button is pressed
        previouslyPressed = currentState;
        return result;
    }

    bool wasJustReleased() {
        bool currentState = isPressed();
        bool result = !currentState && previouslyPressed;  // True only on the first frame the button is released
        previouslyPressed = currentState;
        return result;
    }
};

// === LED Manager ===
// Controls all the LEDs in the system
class LedManager {
private:
    const int* counterLedPins;  // Array of pins for the counter LEDs
    const int ledCount;         // Number of counter LEDs

public:
    LedManager(const int* pins, int count) 
        : counterLedPins(pins), ledCount(count) {
        // Set up all LED pins as outputs and turn them off
        for (int i = 0; i < count; i++) {
            pinMode(counterLedPins[i], OUTPUT);
            digitalWrite(counterLedPins[i], LOW);
        }
        pinMode(LED_ACTION_STARTED_PIN, OUTPUT);
        pinMode(LED_ACTION_COMPLETED_PIN, OUTPUT);
    }

    void turnOffCounterLeds() {
        // Turn off all counter LEDs
        for (int i = 0; i < ledCount; i++) {
            digitalWrite(counterLedPins[i], LOW);
        }
    }

    void updateCounterLeds(int counter) {
        // Calculate how many LEDs should be on (5 - counter)
        int ledsOn = MAX_COUNTER - counter;
        // Turn on the first 'ledsOn' LEDs, turn off the rest
        for (int i = 0; i < ledCount; i++) {
            digitalWrite(counterLedPins[i], (i < ledsOn) ? HIGH : LOW);
        }
    }

    void setActionStarted(bool on) {
        digitalWrite(LED_ACTION_STARTED_PIN, on ? HIGH : LOW);
    }

    void setActionCompleted(bool on) {
        digitalWrite(LED_ACTION_COMPLETED_PIN, on ? HIGH : LOW);
    }
};

// === Sound Manager ===
// Handles all sound effects in the system
class SoundManager {
private:
    const int pin;  // The pin number for the buzzer

public:
    SoundManager(int pin) : pin(pin) {
        pinMode(pin, OUTPUT);
    }

    void playConfirmationBeep() {
        // Play three short beeps to confirm action completion
        for (int i = 0; i < 3; i++) {
            tone(pin, BEEP_FREQUENCY);
            delay(100);
            noTone(pin);
            delay(100);
        }
    }

    void playErrorTone() {
        // Play error tone (high-low sequence)
        tone(pin, BEEP_FREQUENCY);
        delay(150);
        tone(pin, ERROR_FREQUENCY);
        delay(150);
        noTone(pin);
    }

    void playSweepTone(float progress) {
        // Play a sweeping tone during action hold
        int freq = ERROR_FREQUENCY + (progress * (BEEP_FREQUENCY - ERROR_FREQUENCY));
        tone(pin, freq);
    }

    void stopTone() {
        noTone(pin);
    }
};

// === Device System ===
// Main system class that coordinates all components
class DeviceSystem {
private:
    Button deviceButton;    // The main button (D2)
    Button actionButton;    // The action button (D10)
    LedManager leds;       // LED manager
    SoundManager sound;    // Sound manager
    SystemState state = SystemState::NORMAL;  // Current system state
    int counter = 0;       // Current counter value
    bool pendingCounterIncrement = false;  // Flag for pending counter increment
    bool actionInProgress = false;  // Flag for action in progress
    bool actionDoneThisCycle = false;  // Flag for action completed in current cycle
    bool displayCounter = false;  // Flag for whether to display counter
    bool deviceButtonCurrentlyPressed = false;  // Current state of device button
    unsigned long actionStartTime = 0;  // When the current action started
    unsigned long deviceButtonReleaseTime = 0;  // When the device button was released

public:
    DeviceSystem() 
        : deviceButton(DEVICE_BUTTON_PIN)
        , actionButton(ACTION_BUTTON_PIN)
        , leds(COUNTER_LED_PINS, MAX_COUNTER)
        , sound(BUZZER_PIN) {}

    void update() {
        // Main update loop - handles either normal or error state
        if (state == SystemState::ERROR) {
            handleErrorState();
        } else {
            handleNormalState();
        }
    }

private:
    void handleErrorState() {
        // Handle error state - turn off all LEDs and play error tone on button press
        leds.turnOffCounterLeds();
        leds.setActionStarted(false);
        leds.setActionCompleted(false);
        sound.stopTone();

        if (deviceButton.wasJustPressed()) {
            sound.playErrorTone();
        }
    }

    void handleNormalState() {
        // Handle normal operation state
        bool devicePressed = deviceButton.isPressed();
        deviceButtonCurrentlyPressed = devicePressed;

        if (devicePressed) {
            // Device button is pressed
            deviceButtonReleaseTime = millis();
            displayCounter = true;

            if (!actionDoneThisCycle && actionButton.wasJustPressed()) {
                startAction();
            }
        } else {
            // Device button is released
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
        // Start a new action
        actionInProgress = true;
        actionStartTime = millis();
        leds.setActionStarted(true);
    }

    void handleActionInProgress() {
        // Handle action in progress - check hold time and update feedback
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
        // Complete the current action
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
        // Cancel the current action
        actionInProgress = false;
        leds.setActionStarted(false);
        leds.setActionCompleted(false);
        sound.stopTone();
        pendingCounterIncrement = false;
        Serial.println("Action cancelled");
    }

    void updateDisplay() {
        // Update the counter LED display
        if (displayCounter) {
            leds.updateCounterLeds(counter);
        } else {
            leds.turnOffCounterLeds();
        }
    }

    void enterErrorState() {
        // Enter error state
        state = SystemState::ERROR;
        Serial.println("Entering Error State!");
        leds.turnOffCounterLeds();
        leds.setActionStarted(false);
        leds.setActionCompleted(false);
        sound.stopTone();
    }
};

// Create the main system instance
DeviceSystem deviceSystem;

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(9600);
}

void loop() {
    // Main program loop - update the system every frame
    deviceSystem.update();
}
