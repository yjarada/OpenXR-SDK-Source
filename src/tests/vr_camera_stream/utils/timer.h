#pragma once

#include <chrono>

class Timer {
public:
    Timer();
    
    void Start();
    void Stop();
    double GetElapsedMilliseconds() const;
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;
    bool running_;
};