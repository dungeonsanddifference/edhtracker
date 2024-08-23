/*
 * Code for tracking Life Total, Poison Counters, and Commander Damage
 * in EDH/Commander. Uses a SSD1306 OLED Display and a rotary encoder
 * with push-button.
 */

#define USE_SSD1306      // Select display driver (SSD1306 ONLY SO FAR)
#define USE_I2C_ENCODER  // Comment this out to switch to pin-based encoder

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "RotaryEncoder.h"   // Handle encoder input
#include "Counter.h"         // Class for tracking counters

// TODO: Handle this in platformio.ini
// Display settings
#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#ifdef USE_SSD1306
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#else
#include <Adafruit_SH110X.h>
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

// Input Settings
#ifdef USE_I2C_ENCODER
I2CRotaryEncoder rotaryEncoder;  // I2C implementation
#else
#define ROTARY_PIN_A 14
#define ROTARY_PIN_B 15
#define BUTTON_PIN 16
PinRotaryEncoder rotaryEncoder(ROTARY_PIN_A, ROTARY_PIN_B, BUTTON_PIN);  // Pin-based implementation
#endif

#define MAX_OPPONENTS 5
#define BAUDRATE 115200

Counter lifeTotal("Life Total", 40, 0, 100, 0, true);
Counter poisonCounters("Poison Counters", 0, 0, 10, 10);
Counter* commanderDamage = nullptr; // Placeholder before selecting number of opponents

Counter** counters = nullptr;  // Pointer to an array of pointers to Counter objects
uint8_t numCounters = 2;  // Initially, we have lifeTotal and poisonCounters
uint8_t currentCounterIndex = 0;
uint8_t numOpponents = 0;

int16_t oldPosition  = 0;  // To track rotary encoder changes

// For non-blocking button handling
bool isButtonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;  // Debounce delay in milliseconds

// Function prototypes
void handleRotation();
void selectNumOpponents();
void setupCommanderDamageCounters();
void displayCounter();

// Setup
void setup() {
    Serial.begin(BAUDRATE);

    // Initialize SSD1306 display
    if(!display.begin(OLED_I2C_ADDRESS, OLED_RESET)) {
        Serial.println(F("OLED allocation failed"));
        for(;;); // Don't proceed, loop forever because we're dead in the water without the display
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(1); // These are monochrome screens

    // Select number of opponents
    selectNumOpponents();

    // Display the initial counter
    displayCounter();
}

// Main loop
void loop() {
    // Check for rotation
    handleRotation();
    // Presing the button will cycle between counters
    if (rotaryEncoder.isButtonPressed()) {
        currentCounterIndex = (currentCounterIndex + 1) % numCounters;
        displayCounter();
    }
}

// Non-blocking rotation handling
void handleRotation() {
    int16_t newPosition = rotaryEncoder.read();

    // Check for rotation
    if (newPosition != oldPosition) {
        if (newPosition > oldPosition) {
            counters[currentCounterIndex]->increment();
        } else if (newPosition < oldPosition) {
            counters[currentCounterIndex]->decrement();
        }
        oldPosition = newPosition;
        displayCounter();
    }
}

// Loop for selecting number of opponents
void selectNumOpponents() {
    bool selectingOpponents = true;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("Select Opponents");
    display.display();

    while (selectingOpponents) {
        int16_t newPosition = rotaryEncoder.read();

        // Update the number of opponents based on encoder rotation
        if (newPosition != oldPosition) {
            if (newPosition > oldPosition && numOpponents < MAX_OPPONENTS) {
                numOpponents++;
            } else if (newPosition < oldPosition && numOpponents > 1) {
                numOpponents--;
            }
            oldPosition = newPosition;

            // Update display with the current number of opponents
            display.clearDisplay();
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.print("Select Opponents");
            display.setCursor(0, 20);
            display.setTextSize(3);
            display.print(numOpponents);
            display.display();
        }

        // Check for button press to confirm the selection
        if (rotaryEncoder.isButtonPressed()) {
            selectingOpponents = false;
        }
    }

    setupCommanderDamageCounters();
}

void setupCommanderDamageCounters() {
    numCounters = 2 + numOpponents;  // Update total number of counters

    // Allocate memory for all counters, including lifeTotal, poisonCounters, and commanderDamage counters
    counters = new Counter*[numCounters];

    // Assign lifeTotal and poisonCounters to the counters array
    counters[0] = &lifeTotal;
    counters[1] = &poisonCounters;

    // Allocate memory for commander damage counters and assign them to the counters array
    commanderDamage = new Counter[numOpponents];
    for (uint8_t i = 0; i < numOpponents; i++) {
        String name = "Cmdr Dmg: Opp " + String(i + 1);
        commanderDamage[i] = Counter(name.c_str(), 0, 0, 21, 100);  // Initialize each with commander damage rules
        counters[2 + i] = &commanderDamage[i];  // Assign to the counters array
    }
}

void displayCounter() {
    display.clearDisplay();

    Counter* counter = counters[currentCounterIndex];

    display.setCursor(0, 0);
    display.print(counter->getName());

    display.setCursor(0, 20);
    display.setTextSize(3);  // Larger text size for the counter value
    display.print(counter->currentCount);

    display.display();  // Update the display with the new information
}
