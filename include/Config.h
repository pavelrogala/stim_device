
#ifndef CONFIG_H
#define CONFIG_H

constexpr int DEVICE_BUTTON_PIN = 2;
constexpr int ACTION_BUTTON_PIN = 10;
constexpr int LED_ACTION_STARTED_PIN = 13;
constexpr int LED_ACTION_COMPLETED_PIN = 8;
constexpr int BUZZER_PIN = 9;
constexpr int COUNTER_LED_PINS[5] = {3, 4, 5, 6, 7};

constexpr int MAX_COUNTER = 5;
constexpr unsigned long ACTION_HOLD_TIME_MS = 2000;
constexpr unsigned long DISPLAY_TIMEOUT_MS = 2500;

constexpr int BEEP_FREQUENCY = 600;
constexpr int ERROR_FREQUENCY = 300;
constexpr unsigned long BEEP_DURATION = 100;
constexpr unsigned long BEEP_PAUSE = 100;
constexpr unsigned long ERROR_TONE_DURATION = 150;
constexpr int CONFIRMATION_BEEPS = 3;

constexpr int LED_ANIMATION_SPEED = 60;
constexpr int LED_FLICKER_MIN = 30;
constexpr int LED_FLICKER_MAX = 60;
constexpr int LED_FLICKER_MIN_COUNT = 3;
constexpr int LED_FLICKER_MAX_COUNT = 6;
constexpr unsigned long ERROR_LED_FLASH_DURATION = 150;

#endif
