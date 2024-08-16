#include "Counter.h"

Counter::Counter(const char* name, uint16_t current, uint16_t min, uint16_t max, uint16_t lethal, bool inverted)
    : name(name), currentCount(current), minCount(min), maxCount(max), lethalCount(lethal), isInverted(inverted), isDead(false) {
    updateIsDead();  // Ensure isDead is set correctly when the object is created
}

void Counter::increment() {
    if (currentCount < maxCount) {
        currentCount++;
    }
    updateIsDead();  // Update isDead after changing currentCount
}

void Counter::decrement() {
    if (currentCount > minCount) {
        currentCount--;
    }
    updateIsDead();  // Update isDead after changing currentCount
}

void Counter::updateIsDead() {
    if (isInverted) {
        isDead = currentCount <= lethalCount;  // Lethal if the count is below or equal to lethal for inverted counters
    } else {
        isDead = currentCount >= lethalCount;  // Lethal if the count is above or equal to lethal for normal counters
    }
}

const char* Counter::getName() {
    return name;
}
