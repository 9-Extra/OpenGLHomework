#pragma once
#include <chrono>

class Clock {
private:
    std::chrono::time_point<std::chrono::steady_clock> now;

public:
    Clock() : now(std::chrono::steady_clock::now()) {}

    float update() {
        using namespace std::chrono;
        float delta = duration_cast<duration<float, std::milli>>(
                          steady_clock::now() - now)
                          .count();
        now = std::chrono::steady_clock::now();
        return delta;
    }
    float get_current_delta() const{
        using namespace std::chrono;
        return duration_cast<duration<float, std::milli>>(steady_clock::now() - now).count();
    }
};