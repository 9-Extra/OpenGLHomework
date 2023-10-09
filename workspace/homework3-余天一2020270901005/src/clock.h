#pragma once

#include <chrono>

class Clock {
private:
    std::chrono::time_point<std::chrono::steady_clock> now;
    float delta;

public:
    Clock() : now(std::chrono::steady_clock::now()) {}

    void update() {
        using namespace std::chrono;
        delta = duration_cast<duration<float, std::milli>>(steady_clock::now() - now).count();
        now = std::chrono::steady_clock::now();
    }

    float get_delta() const { return delta; } // 返回该帧相对上一帧过去的以float表示的毫秒数
    float get_current_delta() const // 获得调用此函数的时间相对上一帧过去的以float表示的毫秒数
    {
        using namespace std::chrono;
        return duration_cast<duration<float, std::milli>>(steady_clock::now() - now).count();
    }
};