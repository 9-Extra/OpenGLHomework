#pragma once

#include <atomic>
#include "winapi.h"
#include "clock.h"
#include "World.h"
#include "InputHandler.h"
#include "Display.h"
#include "Command.h"
#include "system.h"

class GlobalRuntime {
public:
    bool render_frame = false;

    bool the_world_enable = true;

    std::atomic_int32_t fps;

    DWORD main_thread_id;

    Display display;
    MenuManager menu;
    InputHandler input;

    Renderer renderer;
    World world;
    
    Clock logic_clock, render_clock;
    CommandContainer command_stack;

    std::vector<std::unique_ptr<ISystem>> system_list;

    GlobalRuntime(uint32_t width = 1080, uint32_t height = 720)
        : main_thread_id(GetCurrentThreadId()), display(width, height), world(renderer){
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

    void execute_command(std::unique_ptr<Command>&& command){
        command->invoke();
        command_stack.push_command(std::move(command));
    }
};

extern std::unique_ptr<GlobalRuntime> runtime;

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