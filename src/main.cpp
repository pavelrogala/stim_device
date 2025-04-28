#include <Arduino.h>

// Pin definitions
const int deviceActivationButtonPin = 2; // D2
const int actionButtonPin = 10;           // D10
const int ledActionStartedPin = 13;       // D13
const int ledActionCompletedPin = 8;      // D8
const int buzzerPin = 9;                  // D9

const int counterLedPins[5] = {3, 4, 5, 6, 7}; // D3-D7

// Variables
int counter = 0;
bool errorState = false;

unsigned long deviceButtonReleaseTime = 0;
bool deviceButtonPreviouslyPressed = false;
bool actionAlreadyDoneThisPress = false;

// Function to turn off all counter LEDs
void turnOffCounterLeds() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(counterLedPins[i], LOW);
  }
}

// Function to update counter LEDs based on counter value
void updateCounterLeds() {
  int ledsToLight = 5 - counter;
  for (int i = 0; i < 5; i++) {
    digitalWrite(counterLedPins[i], (i < ledsToLight) ? HIGH : LOW);
  }
}

// Function to play error tone (after reaching counter 5)
void playErrorTone() {
  tone(buzzerPin, 700);
  delay(150);
  tone(buzzerPin, 300);
  delay(150);
  noTone(buzzerPin);
}

// Function to play success beeps (after valid action)
void playSuccessBeeps() {
  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, 300);
    delay(100);
    noTone(buzzerPin);
    delay(100);
  }
}

void setup() {
  pinMode(deviceActivationButtonPin, INPUT_PULLUP);
  pinMode(actionButtonPin, INPUT_PULLUP);
  pinMode(ledActionStartedPin, OUTPUT);
  pinMode(ledActionCompletedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  for (int i = 0; i < 5; i++) {
    pinMode(counterLedPins[i], OUTPUT);
    digitalWrite(counterLedPins[i], LOW);
  }

  digitalWrite(ledActionStartedPin, LOW);
  digitalWrite(ledActionCompletedPin, LOW);

  Serial.begin(9600);
}

void loop() {
  bool deviceButtonPressed = digitalRead(deviceActivationButtonPin) == LOW;
  bool actionButtonPressed = digitalRead(actionButtonPin) == LOW;
  static bool actionInProgress = false;
  static unsigned long actionStartTime = 0;
  static bool sweeping = false;

  // First, handle Error State
  if (errorState) {
    if (deviceButtonPressed && !deviceButtonPreviouslyPressed) {
      playErrorTone();
    }
    deviceButtonPreviouslyPressed = deviceButtonPressed;
    return; // No other interaction allowed
  }

  // Manage Counter LEDs visibility
  if (deviceButtonPressed) {
    updateCounterLeds();
    deviceButtonReleaseTime = millis(); // Reset the 5s timer
  } else {
    // After release, keep LEDs on for 5 seconds
    if (millis() - deviceButtonReleaseTime < 5000) {
      updateCounterLeds();
    } else {
      turnOffCounterLeds();
    }
  }

  // Only allow action logic if device button is pressed
  if (deviceButtonPressed && !actionAlreadyDoneThisPress) {
    if (actionButtonPressed && !actionInProgress) {
      // Start new action
      actionStartTime = millis();
      actionInProgress = true;
      sweeping = true;
      digitalWrite(ledActionStartedPin, HIGH);
    }
  }

  if (actionInProgress) {
    if (!actionButtonPressed) {
      // Action button released too early, cancel action
      actionInProgress = false;
      sweeping = false;
      digitalWrite(ledActionStartedPin, LOW);
      noTone(buzzerPin);
    } else {
      unsigned long heldTime = millis() - actionStartTime;

      if (heldTime >= 2000) {
        // Action completed after 2s
        actionInProgress = false;
        sweeping = false;
        actionAlreadyDoneThisPress = true; // Prevent re-trigger during same D2 press

        digitalWrite(ledActionStartedPin, LOW);
        digitalWrite(ledActionCompletedPin, HIGH);

        noTone(buzzerPin); // Stop sweep

        if (counter < 5) {
          counter++;
          playSuccessBeeps();
          Serial.print("Counter incremented: ");
          Serial.println(counter);
        }

        if (counter >= 5) {
          errorState = true;
          Serial.println("Entering Error State!");
        }
      } else {
        // During 2s hold, sweeping tone from 700Hz to 300Hz
        if (sweeping) {
          int sweepFreq = 700 - (400.0 * heldTime / 2000);
          tone(buzzerPin, sweepFreq);
        }
      }
    }
  }

  if (!deviceButtonPressed) {
    digitalWrite(ledActionStartedPin, LOW);
    digitalWrite(ledActionCompletedPin, LOW);
    actionAlreadyDoneThisPress = false; // Reset permission when D2 is released
  }

  deviceButtonPreviouslyPressed = deviceButtonPressed;
}
