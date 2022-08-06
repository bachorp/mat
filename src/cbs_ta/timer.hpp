#pragma once

#include <chrono>
#include <iostream>

class Timer {
public:
    Timer()
            : start_(std::chrono::high_resolution_clock::now()),
              end_(std::chrono::high_resolution_clock::now()),
              stopped(false) {}

    void reset() {
        stopped = false;
        start_ = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        stopped = true;
        end_ = std::chrono::high_resolution_clock::now();
    }

    double elapsedSeconds() const {
        if (stopped) {
            auto timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(
                    end_ - start_);
            return timeSpan.count();
        }
        auto timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(
                std::chrono::high_resolution_clock::now() - start_);
        return timeSpan.count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
    std::chrono::high_resolution_clock::time_point end_;
    bool stopped;
};

class ScopedTimer : public Timer {
public:
    ScopedTimer() {}

    ~ScopedTimer() {
        stop();
        std::cout << "Elapsed: " << elapsedSeconds() << " s" << std::endl;
    }
};

std::ostream &operator<<(std::ostream &os, const Timer &timer) {
    os << timer.elapsedSeconds() << "s";
    return os;
}
