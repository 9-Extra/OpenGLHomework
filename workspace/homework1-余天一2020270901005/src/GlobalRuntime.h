#pragma once

#include <atomic>
#include "winapi.h"
#include "clock.h"
#include "World.h"
#include "InputHandler.h"
#include "Display.h"
#include "Command.h"

class GlobalRuntime {
public:
    bool render_frame = false;

    bool the_world_enable = true;

    std::atomic_int32_t fps;

    DWORD main_thread_id;
    uint32_t window_width;
    uint32_t window_height;

    Display display;
    InputHandler input;

    Renderer renderer;
    World world;
    
    Clock logic_clock, render_clock;
    CommandContainer command_stack;

    GlobalRuntime(uint32_t width = 1080, uint32_t height = 720)
        : main_thread_id(GetCurrentThreadId()), window_width(width),
          window_height(height), display(width, height), world(renderer){
        renderer.start_thread();
    }

    void the_world() { the_world_enable = true; }
    void continue_run() {
        the_world_enable = false;
        logic_clock.update(); // skip time
        input.clear_keyboard_state();
    }

    void terminal() {
        // 通知渲染线程退出，但是继续执行，直到渲染线程在退出时通知主线程结束消息循环
        renderer.terminal_thread();
    }
};

extern std::unique_ptr<GlobalRuntime> runtime;