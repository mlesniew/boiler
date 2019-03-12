#ifndef TIMER_H
#define TIMER_H

struct Stopwatch {
    Stopwatch() : start_time(millis()) {}
    virtual ~Stopwatch() {}

    void reset() {
        start_time = millis();
    }

    unsigned long elapsed() const {
        return millis() - start_time;
    }

    unsigned long start_time;
};

struct Periodic : public Stopwatch {
    Periodic(
            unsigned long interval,
            std::function<void()> callback = nullptr,
            bool run_immediately = false)
        : interval(interval), callback(callback) {
        if (run_immediately)
            callback();
    }

    void tick() {
        if (elapsed() < interval)
            return;
        if (callback)
            callback();
        reset();
    }

    unsigned long interval;
    std::function<void()> callback;
};

#endif
