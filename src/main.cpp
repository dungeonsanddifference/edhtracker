/*
   Code for tracking Life Total, Poison Counters, and Commander Damage
   in EDH/Commander. Uses a SH1106 OLED Display and a rotary encoder
   with push-button.
*/

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>     
#include <RotaryEncoder.h>     
#include "Counter.h"     
#include "Sprites.h"

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

// Create the rotary encoder
RotaryEncoder encoder(PIN_ROTA, PIN_ROTB, RotaryEncoder::LatchMode::FOUR3);
void checkPosition() { encoder.tick(); }

// Initialize display
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);  // Use hardware I2C

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

  // Initialize interrupts
  pinMode(PIN_ROTA, INPUT_PULLUP);
  pinMode(PIN_ROTB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTA), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTB), checkPosition, CHANGE);  
  
  // Initialize button, LED pins
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // Initialize SH1106 display
  display.begin();
  display.clearBuffer();
  display.drawBitmap(-10, 0, SPLASH_WIDTH/8, SPLASH_HEIGHT, splash_data);
  display.setFont(u8g2_font_ncenB08_tr);
  display.setCursor(54, 10);
  display.print("Commander");
  display.setCursor(54,22);
  display.print("Tracker");
  display.sendBuffer();
  delay(5000);

  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr); // Set font for text display
  display.drawStr(0, 10, "Select Opponents");
  display.setFont(u8g2_font_ncenB14_tr); // Larger font for numbers
  display.drawStr(0, 30, String(numOpponents).c_str());
  display.sendBuffer();

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
    
    // Update display
    displayCounter();
    
    // Check if any of the counters are lethal and power LED if so
    if (checkIfAnyCounterIsDead()) {
      digitalWrite(PIN_LED, HIGH);
    } else {
      digitalWrite(PIN_LED, LOW);
    }
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
      display.clearBuffer();
      display.setFont(u8g2_font_ncenB08_tr);
      display.drawStr(0, 10, "Select Opponents");
      display.setFont(u8g2_font_ncenB14_tr);
      display.drawStr(0, 30, String(numOpponents).c_str());
      display.sendBuffer();
    }

    // Check for button press to confirm the selection
    if (digitalRead(PIN_BTN) == LOW && (millis() - lastDebounceTime) > debounceDelay) {
      lastDebounceTime = millis();
      selectingOpponents = false;
    }
  }

  // Add opponent Commander Damage counters to array
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

// Handle display upon counter cycling/update
void displayCounter() {
  display.clearBuffer();

  Counter* counter = counters[currentCounterIndex];

  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 10, counter->getName());

  display.setFont(u8g2_font_ncenB14_tr);  // Larger font for the counter value
  display.setCursor(0, 30);
  display.print(counter->currentCount);

  display.sendBuffer();  // Update the display with the new information
}
