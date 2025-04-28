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
bool errorState = false;

void setup() {
  // Initialize pins
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  for (int i = 0; i < 10; i++) {
    pinMode(ledBarPins[i], OUTPUT);
    digitalWrite(ledBarPins[i], HIGH);
  }

  Serial.begin(9600);
}

void updateLedBar(int value) {
  for (int i = 0; i < 10; i++) {
    digitalWrite(ledBarPins[i], (i < value) ? LOW : HIGH);
  }
}

void playErrorTone() {
  tone(buzzerPin, 500);
  delay(150);
  tone(buzzerPin, 300);
  delay(150);
  noTone(buzzerPin);
}

void loop() {
  static unsigned long buttonPressStart = 0;
  static unsigned long lastButtonChange = 0;
  static bool counting = false;
  static bool secondLedOn = false;
  static bool lastButtonState = HIGH;
  static bool sweepingTone = false;
  static bool countedThisPress = false;

  bool currentButtonState = digitalRead(buttonPin);

  // Debounce
  if (currentButtonState != lastButtonState) {
    lastButtonChange = millis();
    lastButtonState = currentButtonState;
  }

  if ((millis() - lastButtonChange) > 50) { // Debounce
    if (errorState) {
      if (currentButtonState == LOW && !counting) {
        // Button just pressed -> play error tone immediately
        playErrorTone();
        counting = true;
      } else if (currentButtonState == HIGH && counting) {
        // Button released -> ready to detect next press
        counting = false;
      }
      return; // Skip all normal logic when in error state
    }
    

    if (currentButtonState == LOW) {
      // Button pressed
      if (!counting) {
        buttonPressStart = millis();
        counting = true;
        secondLedOn = false;
        sweepingTone = true;
        countedThisPress = false;
      }

      unsigned long heldTime = millis() - buttonPressStart;

      if (heldTime < 2000) {
        digitalWrite(ledPin1, HIGH);
        digitalWrite(ledPin2, LOW);

        if (sweepingTone) {
          int sweepFrequency = 300 + (700.0 * heldTime / 2000);
          tone(buzzerPin, sweepFrequency);
        }
      } else {
        if (!secondLedOn && !countedThisPress) {
          noTone(buzzerPin);
          sweepingTone = false;

          if (counter < 5) {
            counter++;
            Serial.print("Counter: ");
            Serial.println(counter);
          }

          // Confirmation 3-beep
          for (int i = 0; i < 3; i++) {
            tone(buzzerPin, 1000);
            delay(100);
            noTone(buzzerPin);
            delay(100);
          }

          secondLedOn = true;
          countedThisPress = true;
        }

        digitalWrite(ledPin1, LOW);
        digitalWrite(ledPin2, HIGH);
      }
    } else {
      // Button released
      if (countedThisPress && counter >= 5) {
        errorState = true; // Enter errorState after releasing 5th time
        Serial.println("Entering error state!");
      }

      digitalWrite(ledPin1, LOW);
      digitalWrite(ledPin2, LOW);
      noTone(buzzerPin);
      counting = false;
      sweepingTone = false;
      secondLedOn = false;
    }
  }

  // Update LEDs only if not in error state
  if (!errorState) {
    updateLedBar(counter);
  }
}
