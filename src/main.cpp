#include <Arduino.h>

// Pin definitions
const int deviceButtonPin = 2;    // Device activation button (D2)
const int actionButtonPin = 10;   // Action button (D10)
const int ledActionStartedPin = 13; // LED for action started (D13)
const int ledActionCompletedPin = 8; // LED for action completed (D8)
const int buzzerPin = 9;           // Piezo buzzer (D9)
const int counterLedPins[5] = {3, 4, 5, 6, 7}; // Counter LEDs

// Variables
int counter = 0;
bool errorState = false;
bool deviceButtonPreviouslyPressed = false;
bool actionButtonPreviouslyPressed = false;
bool actionDoneThisCycle = false;
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
    tone(buzzerPin, 300);
    delay(100);
    noTone(buzzerPin);
    delay(100);
  }
}

void playSweepTone(unsigned long durationMs) {
  unsigned long startTime = millis();
  while (millis() - startTime < durationMs) {
    float progress = (float)(millis() - startTime) / durationMs;
    int freq = 300 + (progress * (700 - 300));
    tone(buzzerPin, freq);
    delay(10);
  }
  noTone(buzzerPin);
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
    return; // Nothing else allowed
  }

  // Normal behavior
  if (deviceButtonPressed) {
    deviceButtonReleaseTime = millis(); // Reset release timer
    displayCounter = true;

    if (!actionDoneThisCycle && actionButtonPressed) {
      actionDoneThisCycle = true; // Only allow once per D2 press
      digitalWrite(ledActionStartedPin, HIGH);

      // Start sweep tone and wait for 2s
      playSweepTone(2000);

      // After 2s
      digitalWrite(ledActionStartedPin, LOW);
      digitalWrite(ledActionCompletedPin, HIGH);

      if (counter < 5) {
        counter++;
        Serial.print("Counter incremented to: ");
        Serial.println(counter);
      }

      playConfirmationBeep();
    }
  } else {
    if (millis() - deviceButtonReleaseTime >= 5000) {
      displayCounter = false;
    }
    actionDoneThisCycle = false;
    digitalWrite(ledActionStartedPin, LOW);
    // No change to ledActionCompletedPin here; we'll handle it below based on D10
  }

  // New: Turn OFF D8 when action button is released
  if (!actionButtonPressed && actionButtonPreviouslyPressed) {
    digitalWrite(ledActionCompletedPin, LOW);
  }

  if (displayCounter) {
    updateCounterLeds();
  } else {
    turnOffCounterLeds();
  }

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
