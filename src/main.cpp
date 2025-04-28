#include <Arduino.h>

// === PIN CONFIGURATION ===
// These numbers tell the Arduino which pins are connected to which components.
// If you change the wiring, update these numbers to match.
constexpr int DEVICE_BUTTON_PIN = 2;    // Main button (D2) - activates the device
constexpr int ACTION_BUTTON_PIN = 10;   // Action button (D10) - triggers the counter
constexpr int LED_ACTION_STARTED_PIN = 13;  // LED that shows when action is in progress
constexpr int LED_ACTION_COMPLETED_PIN = 8; // LED that shows when action is completed
constexpr int BUZZER_PIN = 9;           // Buzzer for sound effects
constexpr int COUNTER_LED_PINS[5] = {3, 4, 5, 6, 7};  // The 5 LEDs that show the counter

// === BEHAVIOR CONFIGURATION ===
// These values control how the device behaves. You can change these to adjust:
// - How many actions are needed before error state
// - How long buttons need to be held
// - How the device sounds and looks
constexpr int MAX_COUNTER = 5;          // Number of actions needed before error state
constexpr unsigned long ACTION_HOLD_TIME_MS = 2000;  // How long to hold D10 for action complete (in milliseconds)
constexpr unsigned long DISPLAY_TIMEOUT_MS = 5000;   // How long to show counter after releasing D2 (in milliseconds)

// === SOUND CONFIGURATION ===
constexpr int BEEP_FREQUENCY = 700;     // Frequency of the confirmation beep (in Hz)
constexpr int ERROR_FREQUENCY = 300;    // Frequency of the error tone (in Hz)
constexpr unsigned long BEEP_DURATION = 100;    // How long each beep lasts (in milliseconds)
constexpr unsigned long BEEP_PAUSE = 100;       // Pause between beeps (in milliseconds)
constexpr unsigned long ERROR_TONE_DURATION = 150;  // How long each error tone lasts (in milliseconds)
constexpr int CONFIRMATION_BEEPS = 3;   // Number of beeps for confirmation

// === LED ANIMATION CONFIGURATION ===
constexpr int LED_ANIMATION_SPEED = 150;  // Speed of LED animations (in milliseconds)
constexpr int LED_FLICKER_MIN = 30;     // Minimum flicker duration (in milliseconds)
constexpr int LED_FLICKER_MAX = 120;    // Maximum flicker duration (in milliseconds)
constexpr int LED_FLICKER_MIN_COUNT = 3;  // Minimum number of flickers
constexpr int LED_FLICKER_MAX_COUNT = 6;  // Maximum number of flickers
constexpr unsigned long ERROR_LED_FLASH_DURATION = 150;  // Duration of error LED flashes (in milliseconds)

// === SYSTEM STATES ===
// The device can be in one of three states:
// SETUP: Initial configuration mode
// NORMAL: Regular operation
// ERROR: When counter reaches maximum
enum class SystemState {
    SETUP,
    NORMAL,
    ERROR
};

// === BUTTON CLASS ===
// Handles button input and detects when buttons are pressed or released
class Button {
private:
    const int pin;  // Which pin this button is connected to
    bool previouslyPressed = false;  // Remembers if button was pressed last time

public:
    Button(int pin) : pin(pin) {
        pinMode(pin, INPUT_PULLUP);  // Set up the button pin
    }

    bool isPressed() const {
        return digitalRead(pin) == LOW;  // Button is pressed when pin reads LOW
    }

    bool wasJustPressed() {
        bool currentState = isPressed();
        bool result = currentState && !previouslyPressed;  // True only when button is first pressed
        previouslyPressed = currentState;
        return result;
    }

    bool wasJustReleased() {
        bool currentState = isPressed();
        bool result = !currentState && previouslyPressed;  // True only when button is first released
        previouslyPressed = currentState;
        return result;
    }
};

// === LED MANAGER ===
// Controls all the LEDs in the system
class LedManager {
private:
    const int* counterLedPins;  // Array of pins for the counter LEDs
    const int ledCount;         // Number of counter LEDs
    int lastCounter = -1;       // Remembers last counter value

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
        lastCounter = -1;
    }

    void updateCounterLeds(int counter) {
        int ledsOn = MAX_COUNTER - counter;  // Calculate how many LEDs should be on

        // If counter increased, flicker the LED that just turned off
        if (lastCounter != -1 && counter > lastCounter) {
            int ledToFlicker = MAX_COUNTER - lastCounter - 1;
            if (ledToFlicker >= 0 && ledToFlicker < ledCount) {
                int flickers = random(LED_FLICKER_MIN_COUNT, LED_FLICKER_MAX_COUNT);  // Random number of flickers
                for (int i = 0; i < flickers; i++) {
                    digitalWrite(counterLedPins[ledToFlicker], LOW);
                    delay(random(LED_FLICKER_MIN, LED_FLICKER_MAX));  // Random flicker duration
                    digitalWrite(counterLedPins[ledToFlicker], HIGH);
                    delay(random(LED_FLICKER_MIN, LED_FLICKER_MAX));
                }
                digitalWrite(counterLedPins[ledToFlicker], LOW);
            }
        }

        // Update all LEDs based on counter value
        for (int i = 0; i < ledCount; i++) {
            if (i < ledsOn) {
                digitalWrite(counterLedPins[i], HIGH);
            } else {
                digitalWrite(counterLedPins[i], LOW);
            }
        }

        lastCounter = counter;
    }

    void animateCounterLeds(int counter) {
        // Animate LEDs turning on one by one
        int ledsOn = MAX_COUNTER - counter;
        for (int i = 0; i < ledsOn; i++) {
            digitalWrite(counterLedPins[i], HIGH);
            delay(LED_ANIMATION_SPEED);  // Control animation speed
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
        // Show setup progress on counter LEDs
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

// === SOUND MANAGER ===
// Handles all sound effects in the system
class SoundManager {
private:
    const int pin;  // Which pin the buzzer is connected to

public:
    SoundManager(int pin) : pin(pin) {
        pinMode(pin, OUTPUT);
    }

    void playConfirmationBeep() {
        // Play three short beeps to confirm action completion
        for (int i = 0; i < CONFIRMATION_BEEPS; i++) {
            tone(pin, BEEP_FREQUENCY);
            delay(BEEP_DURATION);
            noTone(pin);
            delay(BEEP_PAUSE);
        }
    }

    void playErrorTone() {
        // Play error tone (high-low sequence)
        tone(pin, BEEP_FREQUENCY);
        delay(ERROR_TONE_DURATION);
        tone(pin, ERROR_FREQUENCY);
        delay(ERROR_TONE_DURATION);
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

// === DEVICE SYSTEM ===
// Main system class that coordinates all components
class DeviceSystem {
private:
    Button deviceButton;    // Main button (D2)
    Button actionButton;    // Action button (D10)
    LedManager leds;       // LED manager
    SoundManager sound;    // Sound manager
    SystemState state = SystemState::SETUP;  // Current system state
    int counter = 0;       // Current counter value
    int setupCounter = 0;  // Counter for setup mode
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
        // Main update loop - handles current state
        if (state == SystemState::ERROR) {
            handleErrorState();
        } else if (state == SystemState::SETUP) {
            handleSetupState();
        } else {
            handleNormalState();
        }
    }

    void begin() {
        leds.updateSetupDisplay(setupCounter);
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

            // Flash action started LED three times
            for (int i = 0; i < 3; i++) {
                digitalWrite(LED_ACTION_STARTED_PIN, HIGH);
                delay(ERROR_LED_FLASH_DURATION);
                digitalWrite(LED_ACTION_STARTED_PIN, LOW);
                delay(ERROR_LED_FLASH_DURATION);
            }
        }
    }

    void handleSetupState() {
        // Handle setup mode - configure initial counter value
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
        // Handle normal operation state
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

    void playCounterAnimation() {
        leds.animateCounterLeds(counter);
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
    deviceSystem.begin();
}

void loop() {
    // Main program loop - update the system every frame
    deviceSystem.update();
}
