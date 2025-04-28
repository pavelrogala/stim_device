#include <Arduino.h>

// Pin definitions
const int deviceButtonPin = 2;
const int actionButtonPin = 10;
const int ledActionStartedPin = 13;
const int ledActionCompletedPin = 8;
const int buzzerPin = 9;
const int counterLedPins[5] = {3, 4, 5, 6, 7};

// Variables
int counter = 0;
bool errorState = false;
bool deviceButtonPreviouslyPressed = false;
bool actionButtonPreviouslyPressed = false;
bool actionInProgress = false;
bool actionDoneThisCycle = false;
unsigned long actionStartTime = 0;
unsigned long deviceButtonReleaseTime = 0;
bool displayCounter = false;

void setup() {
  pinMode(deviceButtonPin, INPUT_PULLUP);
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
  noTone(buzzerPin);

  Serial.begin(9600);
}

void turnOffCounterLeds() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(counterLedPins[i], LOW);
  }
}

void updateCounterLeds() {
  int ledsOn = 5 - counter;
  for (int i = 0; i < 5; i++) {
    digitalWrite(counterLedPins[i], (i < ledsOn) ? HIGH : LOW);
  }
}

void playConfirmationBeep() {
  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, 700);
    delay(100);
    noTone(buzzerPin);
    delay(100);
  }
}

void playErrorTone() {
  tone(buzzerPin, 700);
  delay(150);
  tone(buzzerPin, 300);
  delay(150);
  noTone(buzzerPin);
}

void loop() {
  bool deviceButtonPressed = digitalRead(deviceButtonPin) == LOW;
  bool actionButtonPressed = digitalRead(actionButtonPin) == LOW;

  // Handle error state
  if (errorState) {
    turnOffCounterLeds();
    digitalWrite(ledActionStartedPin, LOW);
    digitalWrite(ledActionCompletedPin, LOW);
    noTone(buzzerPin);

    if (deviceButtonPressed && !deviceButtonPreviouslyPressed) {
      playErrorTone();
    }
    deviceButtonPreviouslyPressed = deviceButtonPressed;
    actionButtonPreviouslyPressed = actionButtonPressed;
    return;
  }

  // Device button logic
  if (deviceButtonPressed) {
    deviceButtonReleaseTime = millis();
    displayCounter = true;

    if (!actionDoneThisCycle) {
      // Handle action button being pressed
      if (actionButtonPressed && !actionButtonPreviouslyPressed) {
        actionInProgress = true;
        actionStartTime = millis();
        digitalWrite(ledActionStartedPin, HIGH);
      }
    }
  } else {
    if (millis() - deviceButtonReleaseTime >= 5000) {
      displayCounter = false;
    }
    actionDoneThisCycle = false;
    actionInProgress = false;
    digitalWrite(ledActionStartedPin, LOW);
  }

  // Handle action in progress
  if (actionInProgress) {
    unsigned long heldDuration = millis() - actionStartTime;

    // Sweep tone during hold
    float progress = (float)heldDuration / 2000.0;
    if (progress > 1.0) progress = 1.0;
    int freq = 300 + (progress * (700 - 300));
    tone(buzzerPin, freq);

    // If action button released early -> cancel
    if (!actionButtonPressed) {
      actionInProgress = false;
      noTone(buzzerPin);
      digitalWrite(ledActionStartedPin, LOW);
    }

    // If held for 2 seconds -> complete the action
    if (heldDuration >= 2000 && !actionDoneThisCycle) {
      actionDoneThisCycle = true;
      actionInProgress = false;
      noTone(buzzerPin);

      digitalWrite(ledActionStartedPin, LOW);
      digitalWrite(ledActionCompletedPin, HIGH);

      if (counter < 5) {
        counter++;
        Serial.print("Counter incremented to: ");
        Serial.println(counter);
      }

      playConfirmationBeep();
    }
  }

  // Turn OFF ledActionCompletedPin when action button released
  if (!actionButtonPressed && actionButtonPreviouslyPressed) {
    digitalWrite(ledActionCompletedPin, LOW);
  }

  // Update counter LEDs
  if (displayCounter) {
    updateCounterLeds();
  } else {
    turnOffCounterLeds();
  }

  // Enter error state if counter reaches 5 and device button is released
  if (counter >= 5 && !errorState && !deviceButtonPressed) {
    errorState = true;
    Serial.println("Entering Error State!");
    turnOffCounterLeds();
    digitalWrite(ledActionStartedPin, LOW);
    digitalWrite(ledActionCompletedPin, LOW);
    noTone(buzzerPin);
  }

  deviceButtonPreviouslyPressed = deviceButtonPressed;
  actionButtonPreviouslyPressed = actionButtonPressed;
}
