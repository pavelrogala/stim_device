#include <Arduino.h>
#include <Config.h>
#include "ButtonManager.h"
#include "LedManager.h"
#include "SoundManager.h"

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

// === DEVICE SYSTEM ===
// Main system class that coordinates all components
class DeviceSystem {
private:
    ButtonManager deviceButton;    // Main button (D2)
    ButtonManager actionButton;    // Action button (D10)
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
            setupCounter++;
            if (setupCounter >= MAX_COUNTER) {
                setupCounter = 0; // Wrap back to 0 if reaching MAX_COUNTER
            }
            leds.updateSetupDisplay(setupCounter);
            Serial.print("Setup: LEDs turned off = ");
            Serial.println(setupCounter);
        }
    
        static bool gameStarted = false;
        if (!gameStarted && actionButton.wasHeldFor(1000)) {
            gameStarted = true;
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

// Create the main system instance (test git pull)
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
