/* I like Adafruit Stemma QT I2C breakouts for rapid prototyping, so this is a common
 * interface for handling rotary encoders, either the  Adafruit I2C QT Rotary Encoder
 * or one wired directly to the target microcontroller pins. Define USE_I2C_ENCODER to
 * use the former. The default is Encoder.h
 */

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

// Interface for handling directly-wired EC11 rotary encoders or Adafruit Seesaw encoders
class EncoderInterface {
public:
    virtual long read() = 0;  // Pure virtual method for reading encoder events
    virtual bool isButtonPressed() = 0;
};

#ifdef USE_I2C_ENCODER

#include <Adafruit_seesaw.h>

#define SEESAW_ADDR 0x36
#define SS_SWITCH 24

// I2C Rotary Encoder Implementation
class I2CRotaryEncoder : public EncoderInterface {
private:
    Adafruit_seesaw ss;
    int32_t encoderPosition;

public:
    I2CRotaryEncoder() : encoderPosition(0) {
        if (!ss.begin(SEESAW_ADDR)) {
            Serial.println("Couldn't find seesaw on default address");
            while (1) delay(10);  // Stop execution if the seesaw is not found
        }
        ss.pinMode(SS_SWITCH, INPUT_PULLUP);  // Initialize the encoder switch pin
        encoderPosition = ss.getEncoderPosition();
        ss.enableEncoderInterrupt();
    }

    long read() override {
        int32_t newPosition = ss.getEncoderPosition();
        return newPosition;
    }

    bool isButtonPressed() override {
        return !ss.digitalRead(SS_SWITCH);  // Button is pressed when the pin reads LOW
    }
};

#else

// Pin-based Rotary Encoder Implementation for RP2040
class PinRotaryEncoder : public EncoderInterface {
private:
    uint8_t pinA, pinB, buttonPin;
    int16_t encoderPosition;
    bool lastA, lastB;
    bool lastButtonState, debouncedButtonState;
    unsigned long lastDebounceTime;
    const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

public:
    PinRotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t btnPin)
        : pinA(pinA), pinB(pinB), buttonPin(btnPin), encoderPosition(0), 
          lastA(LOW), lastB(LOW), lastButtonState(HIGH), debouncedButtonState(HIGH), 
          lastDebounceTime(0) {

        pinMode(pinA, INPUT);
        pinMode(pinB, INPUT);
        pinMode(buttonPin, INPUT_PULLUP);  // Initialize the button pin with internal pull-up
    }

    long read() override {
        // Non-blocking encoder read
        bool currentA = digitalRead(pinA);
        bool currentB = digitalRead(pinB);

        // Check for encoder rotation
        if (currentA != lastA || currentB != lastB) {
            if (currentA == currentB) {
                encoderPosition++;
            } else {
                encoderPosition--;
            }
        }

        // Save the current state for the next read
        lastA = currentA;
        lastB = currentB;

        return encoderPosition;
    }

    bool isButtonPressed() override {
        // Non-blocking button state read with debounce
        bool currentButtonState = digitalRead(buttonPin);
        if (currentButtonState != lastButtonState) {
            lastDebounceTime = millis();
        }

        if ((millis() - lastDebounceTime) > debounceDelay) {
            if (currentButtonState == LOW && !debouncedButtonState) {
                debouncedButtonState = true;
            } else if (currentButtonState == HIGH) {
                debouncedButtonState = false;
            }
        }

        lastButtonState = currentButtonState;

        return debouncedButtonState;
    }
};

#endif  // USE_I2C_ENCODER

#endif  // ROTARY_ENCODER_H
