#ifndef BLINKER_H
#define BLINKER_H

#include <Ticker.h>

struct Blinker {
    Blinker(DigitalOutput & output, unsigned long interval=500, unsigned long pattern=0b10)
        : output(output) {
        set_pattern(pattern);
        start(interval);
    }

    void set_pattern(unsigned long pattern) {
        if (this->pattern != pattern) {
            this->pattern = pattern;
            restart_pattern();
        }
    }

    unsigned long get_pattern() const {
        return this->pattern;
    }

    void start(unsigned long interval=250) {
        step();
        ticker.attach_ms(interval, step_wrapper, this);
    }

    void stop() {
        ticker.detach();
        output.off();
    }

private:
    void restart_pattern() {
        for (position = 31; position > 0; --position) {
            if ((pattern >> position) & 1)
                break;
        }
    }

    void step() {
        output.set((pattern >> position) & 1);
        if (position-- == 0)
            restart_pattern();
    }

    static void step_wrapper(Blinker * blinker) {
        blinker->step();
    }

    DigitalOutput & output;
    unsigned long pattern;
    unsigned char position;
    Ticker ticker;
};


struct BlinkerSettingGuard {
    BlinkerSettingGuard(Blinker & blinker, unsigned long pattern):
        blinker(blinker), old_pattern(blinker.get_pattern()) {
        blinker.set_pattern(pattern);
    }

    ~BlinkerSettingGuard() {
        blinker.set_pattern(old_pattern);
    }

private:
    Blinker & blinker;
    const unsigned long old_pattern;
};

#endif
