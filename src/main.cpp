/*
 * Code for tracking Life Total, Poison Counters, and Commander Damage
 * in EDH/Commander. Uses a SH1106 OLED Display and a rotary encoder
 * with push-button.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Counter.h"         // Class for tracking counters
#include <Adafruit_SH110X.h> // 2.1.10
#include <RotaryEncoder.h>

#define BAUDRATE 115200

// HW pins
#define PIN_ROTA 6  // rotary encoder A
#define PIN_ROTB 7  // rotary encoder B
#define PIN_BTN  8  // pushbutton
#define PIN_LED  13  // status LED

// Display settings
#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// Create the rotary encoder
RotaryEncoder encoder(PIN_ROTA, PIN_ROTB, RotaryEncoder::LatchMode::FOUR3);
void checkPosition() {  encoder.tick(); } // just call tick() to check the state.

// Initialize display
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Game Rule Settings
#define MAX_OPPONENTS 5

Counter lifeTotal("Life Total", 40, 0, 100, 0, true);
Counter poisonCounters("Poison Counters", 0, 0, 10, 10);
Counter* commanderDamage = nullptr; // Placeholder before selecting number of opponents

Counter** counters = nullptr;  // Pointer to an array of pointers to Counter objects
uint8_t numCounters = 2;  // Initially, we have lifeTotal and poisonCounters
uint8_t currentCounterIndex = 0;
uint8_t numOpponents = 0;

// For non-blocking button handling
bool isButtonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;  // Debounce delay in milliseconds

// Function prototypes
int8_t handleRotation(); // Updated to return +1, 0, or -1
void selectNumOpponents();
void setupCommanderDamageCounters();
bool checkIfAnyCounterIsDead();
void displayCounter();

// Setup
void setup() {
    delay(250);
    Serial.begin(BAUDRATE);
    delay(100);

    // Initialize pins
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    // Initialize SSD1306 display
    if(!display.begin(OLED_I2C_ADDRESS, OLED_RESET)) {
        Serial.println(F("OLED allocation failed"));
        for(;;); // Don't proceed, loop forever because we're dead in the water without the display
    }
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(1); // These are monochrome screens
    display.print("Select Opponents");
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.print(numOpponents);
    display.display();

    // Select number of opponents
    selectNumOpponents();

    // Display the initial counter
    displayCounter();
}

// Main loop
void loop() {
    // Check for rotation
    int8_t rotation = handleRotation();
    if (rotation != 0) {
        // Update the current counter based on rotation direction
        if (rotation == 1) {
            counters[currentCounterIndex]->increment();
        } else if (rotation == -1) {
            counters[currentCounterIndex]->decrement();
        }
        displayCounter();
    }

    // Check for button press to cycle between counters
    if (digitalRead(PIN_BTN) == LOW && (millis() - lastDebounceTime) > debounceDelay) {
        lastDebounceTime = millis();
        currentCounterIndex = (currentCounterIndex + 1) % numCounters;
        displayCounter();
    }
}

// Non-blocking rotation handling
int8_t handleRotation() {
    encoder.tick(); // check the encoder

    // Return direction changes only
    switch (encoder.getDirection()) {
        case RotaryEncoder::Direction::CLOCKWISE:
            return 1; // Clockwise rotation
        case RotaryEncoder::Direction::COUNTERCLOCKWISE:
            return -1; // Counterclockwise rotation
        case RotaryEncoder::Direction::NOROTATION:
        default:
            return 0; // No rotation
    }
}

// Loop for selecting number of opponents
void selectNumOpponents() {
    bool selectingOpponents = true;

    while (selectingOpponents) {
        // Use handleRotation() to get the direction of rotation
        int8_t rotation = handleRotation();
        if (rotation != 0) {
            // Update the number of opponents based on encoder rotation
            if (rotation == 1 && numOpponents < MAX_OPPONENTS) {
                numOpponents++;
            } else if (rotation == -1 && numOpponents > 0) {
                numOpponents--;
            }

            // Update display with the current number of opponents
            display.clearDisplay();
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.print("Select Opponents");
            display.setCursor(0, 20);
            display.setTextSize(2);
            display.print(numOpponents);
            display.display();
        }

        // Check for button press to confirm the selection
        if (digitalRead(PIN_BTN) == LOW && (millis() - lastDebounceTime) > debounceDelay) {
            lastDebounceTime = millis();
            selectingOpponents = false;
        }
    }

    setupCommanderDamageCounters();
}

void setupCommanderDamageCounters() {
    numCounters = 2 + numOpponents;  // Update total number of counters

    // Allocate memory for all counters, including lifeTotal, poisonCounters, and commanderDamage counters
    counters = new Counter*[numCounters];

    // Allocate memory for a stable name buffer
    static char names[MAX_OPPONENTS][13];

    // Assign lifeTotal and poisonCounters to the counters array
    counters[0] = &lifeTotal;
    counters[1] = &poisonCounters;

    // Allocate memory for commander damage counters and assign them to the counters array
    commanderDamage = new Counter[numOpponents];
    for (uint8_t i = 0; i < numOpponents; i++) {
        snprintf(names[i], sizeof(names[i]), "Cmdr Dmg %d", i + 1); // Safely format the name into the buffer
        commanderDamage[i] = Counter(names[i], 0, 0, 100, 21);  // Initialize each with commander damage rules
        counters[2 + i] = &commanderDamage[i];  // Assign to the counters array
    }
}

// Function to check if any counter is dead
bool checkIfAnyCounterIsDead() {
  for (uint8_t i = 0; i < numCounters; i++) {
    if (counters[i]->isDead) {
      return true;
    }
  }
  return false;
}

void displayCounter() {
    display.clearDisplay();

    Counter* counter = counters[currentCounterIndex];

    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print(counter->getName());

    display.setCursor(0, 20);
    display.setTextSize(2);  // Larger text size for the counter value
    display.print(counter->currentCount);

    display.display();  // Update the display with the new information

    // Check if any of the counters are lethal and power LED if so
    if (checkIfAnyCounterIsDead()) {
        digitalWrite(PIN_LED, HIGH);  // Turn on LED if any counter is dead
    } else {
        digitalWrite(PIN_LED, LOW);   // Turn off LED if no counter is dead
    }
}
