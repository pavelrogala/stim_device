#include <Arduino.h>

// Pin definitions
const int buttonPin = 2;
const int ledPin1 = 13;
const int ledPin2 = 8;
const int buzzerPin = 9;

// LED bar pins
const int ledBarPins[10] = {3, 4, 5, 6, 7, 10, 11, 12, A0, A1}; // 10 LEDs

// Variables
int counter = 0;

void setup() {
  // Initialize pins
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  for (int i = 0; i < 10; i++) {
    pinMode(ledBarPins[i], OUTPUT);
    digitalWrite(ledBarPins[i], LOW);
  }

  Serial.begin(9600);
}

void updateLedBar(int value) {
  for (int i = 0; i < 10; i++) {
    digitalWrite(ledBarPins[i], (i < value) ? HIGH : LOW);
  }
}

void loop() {
  static unsigned long buttonPressStart = 0;
  static unsigned long lastButtonChange = 0;
  static bool counting = false;
  static bool secondLedOn = false;
  static bool lastButtonState = HIGH;
  static bool sweepingTone = false;

  bool currentButtonState = digitalRead(buttonPin);

  if (currentButtonState != lastButtonState) {
    lastButtonChange = millis();
    lastButtonState = currentButtonState;
  }

  if ((millis() - lastButtonChange) > 50) { // Debounce
    if (currentButtonState == LOW) {
      if (!counting) {
        buttonPressStart = millis();
        counting = true;
        secondLedOn = false;
        sweepingTone = true; // Start sweeping when button is first pressed
      }

      unsigned long heldTime = millis() - buttonPressStart;

      if (heldTime < 2000) {
        digitalWrite(ledPin1, HIGH);
        digitalWrite(ledPin2, LOW);

        // Sweep tone between 300Hz and 1000Hz over 2 seconds
        if (sweepingTone) {
          int sweepFrequency = 300 + (700.0 * heldTime / 2000); // Linear interpolation
          tone(buzzerPin, sweepFrequency);
        }
      } else {
        if (!secondLedOn) {
          // Stop sweeping
          noTone(buzzerPin);
          sweepingTone = false;

          // Increment counter
          if (counter < 10) {
            counter++;
          }
          Serial.print("Counter: ");
          Serial.println(counter);
          secondLedOn = true;

          // Confirmation 3-beep
          for (int i = 0; i < 3; i++) {
            tone(buzzerPin, 1000);
            delay(100);
            noTone(buzzerPin);
            delay(100);
          }
        }
        digitalWrite(ledPin1, LOW);
        digitalWrite(ledPin2, HIGH);
      }
    } else {
      digitalWrite(ledPin1, LOW);
      digitalWrite(ledPin2, LOW);
      noTone(buzzerPin);
      counting = false;
      sweepingTone = false;
      secondLedOn = false;
    }
  }

  updateLedBar(counter);
}
