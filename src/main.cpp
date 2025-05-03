#include <Arduino.h>
#include "DeviceSystem.h"

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
