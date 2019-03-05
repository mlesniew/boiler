#ifndef DIGITAL_OUTPUT_H
#define DIGITAL_OUTPUT_H

#include "digital_output.h"

struct DigitalOutput {
    explicit DigitalOutput(unsigned int pin, bool inverted=false)
        : pin(pin), inverted(inverted) {
        pinMode(pin, OUTPUT);
        off();
        }

    bool is_on() const {
        return bool(digitalRead(pin)) != inverted;
    }

    void set(bool val) const {
        digitalWrite(pin, (val != inverted) ? HIGH : LOW);
    }

    void on() const {
        set(true);
    }

    void off() const {
        set(false);
    }

    void toggle() const {
        set(!is_on());
    }

    bool operator=(bool & other) const {
        set(other);
        return other;
    }

    operator bool() const {
        return is_on();
    }

    const unsigned int pin;
    const bool inverted;
};

using Led = DigitalOutput;

#endif
