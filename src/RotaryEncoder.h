/* I like Adafruit Stemma QT I2C breakouts for rapid prototyping, so this is a common
 * interface for handling rotary encoders, either the  Adafruit I2C QT Rotary Encoder
 * or one wired directly to the target microcontroller pins. Define USE_I2C_ENCODER to
 * use the former. The default is Encoder.h
 */

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "EncoderInterface.h"

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

#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>

// Pin-based Rotary Encoder Implementation
class PinRotaryEncoder : public EncoderInterface {
private:
    Encoder encoder;
    uint8_t buttonPin;
    bool lastButtonState;
    bool debouncedButtonState;
    unsigned long lastDebounceTime;
    const unsigned long debounceDelay = 200;  // Debounce delay in milliseconds

public:
    PinRotaryEncoder(uint8_t pinA, uint8_t pinB, uint8_t btnPin)
        : encoder(pinA, pinB), buttonPin(btnPin), lastButtonState(false), debouncedButtonState(false), lastDebounceTime(0) {
        pinMode(buttonPin, INPUT_PULLUP);  // Initialize the button pin with internal pull-up
    }

    long read() override {
        return encoder.read();
    }

    bool isButtonPressed() override {
        bool currentButtonState = digitalRead(buttonPin) == LOW;  // Active low

        if (currentButtonState != lastButtonState) {
            lastDebounceTime = millis();  // Reset debounce timer
        }

        if ((millis() - lastDebounceTime) > debounceDelay) {
            if (currentButtonState != debouncedButtonState) {
                debouncedButtonState = currentButtonState;
            }
        }

        lastButtonState = currentButtonState;
        return debouncedButtonState;
    }
};

#endif  // USE_I2C_ENCODER

#endif  // ROTARY_ENCODER_H
