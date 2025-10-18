#include "timer.h"

Timer::Timer() : running_(false) {
}

void Timer::Start() {
    start_time_ = std::chrono::high_resolution_clock::now();
    running_ = true;
}

void Timer::Stop() {
    end_time_ = std::chrono::high_resolution_clock::now();
    running_ = false;
}

double Timer::GetElapsedMilliseconds() const {
    auto end = running_ ? std::chrono::high_resolution_clock::now() : end_time_;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time_);
    return duration.count() / 1000.0;
}