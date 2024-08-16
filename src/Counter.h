#ifndef COUNTER_H
#define COUNTER_H

#include <Arduino.h>

class Counter {
public:
    // Constructor with default values for parameters, including name
    Counter(const char* name = "", uint16_t currentCount = 0, uint16_t minCount = 0, uint16_t maxCount = 100, uint16_t lethalCount = 0, bool isInverted = false);

    void increment();   // Method to increment the count and update isDead
    void decrement();   // Method to decrement the count and update isDead

    uint16_t currentCount;   // Current value of the counter (public)
    bool isDead;             // Public flag to indicate if the counter has reached a lethal state
    const char* getName();   // Method to retrieve the name of the counter

private:
    const char* name;        // Name of the counter
    uint16_t minCount;       // Minimum allowed value
    uint16_t maxCount;       // Maximum allowed value
    uint16_t lethalCount;    // Lethal value that determines when the game is over
    bool isInverted;         // Flag to indicate if the counter is inverted (like Life Total)

    void updateIsDead();     // Private method to update the isDead property
};

#endif
