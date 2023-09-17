#pragma once

#include "command/Command.h"
#include "input/InputHandler.h"
#include "system/ISystem.h"
#include "utils/Clock.h"
#include "utils/winapi.h"
#include "window/Display.h"
#include "window/MenuManager.h"
#include "world/World.h"
#include <atomic>

const float TIME_PERTICK = 1000.0f / 60.0f;

class GlobalRuntime {
public:
    bool the_world_enable = true;
    uint32_t tick_count = 0;
    std::atomic_int32_t fps;

    DWORD main_thread_id;

    Display display;
    MenuManager menu;
    InputHandler input;

    Renderer renderer;
    World world;

    Clock logic_clock, render_clock;
    CommandContainer command_stack;

    GlobalRuntime(uint32_t width = 1080, uint32_t height = 720)
        : main_thread_id(GetCurrentThreadId()), display(width, height), world(renderer) {}

    void the_world() { the_world_enable = true; }
    void continue_run() {
        the_world_enable = false;
        logic_clock.update(); // skip time
        input.clear_keyboard_state();
    }

    void register_system(ISystem *sys) {
        sys->init();
        system_list.emplace_back(std::unique_ptr<ISystem>(sys));
    }

    void execute_command(std::unique_ptr<Command> &&command) {
        command->invoke();
        command_stack.push_command(std::move(command));
    }

    void enter_loop() {
        logic_clock.update();
        while (!pull_events()) {
            tick();
        }
    }

    // 关闭
    void terminal() {
        //通知消息循环退出
        PostQuitMessage(0);
    }

private:
friend void engine_shutdown();
    std::vector<std::unique_ptr<ISystem>> system_list;

    void tick();

    bool pull_events();
};

extern std::unique_ptr<GlobalRuntime> runtime;

void engine_init();
void engine_shutdown();

// class GameObjectAppendCommand: Command{
// protected:
//     uint32_t id;
// public:
//     virtual void invoke(){
//         id = runtime->world.create_object();
//     }
//     virtual void revoke(){
//         runtime->world.kill_object(id);
//     }
//     virtual std::string decription() {return "创建物体";};
// };

// class GameObjectUdateCommand: Command{
// protected:
//     uint32_t id;
//     GObjectDesc before, after;
// public:
//     virtual void invoke(){
//         GObject& obj = runtime->world.objects.at(id);
//         before = obj.to_desc();
//         obj.from_desc(after);
//     }
//     virtual void revoke(){
//         GObject& obj = runtime->world.objects.at(id);
//         after = obj.to_desc();
//         obj.from_desc(before);
//     }
//     virtual std::string decription() {return "更改物体属性";};
// };

// class GameObjectDeleteCommand: Command{
// protected:
//     uint32_t id;
// public:
//     virtual void invoke(){
//         runtime->world.kill_object(id);
//     }
//     virtual void revoke(){
//         id = runtime->world.create_object();
//     }
//     virtual std::string decription() {return "删除物体";};
// };