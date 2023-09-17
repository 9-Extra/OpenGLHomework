#pragma once
#include "engine/GlobalRuntime.h"
#include <stack>

class MoveSystem : public ISystem {
private:
public:
    void handle_mouse() {
        // 使用鼠标中键旋转视角
        //  左上角为(0,0)，右下角为(w,h)
        InputHandler &input = runtime->input;
        static bool is_middle_button_down = false;

        if (!is_middle_button_down && input.is_middle_button_down()) {
            runtime->display.grab_mouse();
        }
        if (is_middle_button_down && !input.is_middle_button_down()) {
            runtime->display.release_mouse();
        }

        if (input.is_middle_button_down()) {
            auto [dx, dy] = input.get_mouse_move().v;
            // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
            const float rotate_speed = 0.003f;
            CameraDesc &desc = runtime->world.set_camera();
            desc.rotation.z += dx * rotate_speed;
            desc.rotation.y -= dy * rotate_speed;
        }

        is_middle_button_down = input.is_middle_button_down();
    }

    void handle_keyboard(float delta) {
        // wasd移动
        World &world = runtime->world;
        const float move_speed = 0.1f * delta;

        InputHandler &input = runtime->input;
        if (input.is_keydown(VK_ESCAPE)) {
            runtime->terminal();
        }
        if (input.is_keydown('W')) {
            Vector3f ori = world.get_camera_oritation();
            ori.y = 0.0;
            world.set_camera().position += ori.normalize() * move_speed;
        }
        if (input.is_keydown('S')) {
            Vector3f ori = world.get_camera_oritation();
            ori.y = 0.0;
            world.set_camera().position += ori.normalize() * -move_speed;
        }
        if (input.is_keydown('A')) {
            Vector3f ori = world.get_camera_oritation();
            ori = {ori.z, 0.0, -ori.x};
            world.set_camera().position += ori.normalize() * move_speed;
        }
        if (input.is_keydown('D')) {
            Vector3f ori = world.get_camera_oritation();
            ori = {ori.z, 0.0, -ori.x};
            world.set_camera().position += ori.normalize() * -move_speed;
        }
        if (input.is_keydown(VK_SPACE)) {
            world.set_camera().position.y += move_speed;
        }
        if (input.is_keydown(VK_SHIFT)) {
            world.set_camera().position.y -= move_speed;
        }

        if (input.is_keydown('0')) {
            world.set_camera().position = {0.0, 0.0, 0.0};
            world.set_camera().rotation = {0.0, 0.0, 0.0};
        }
    }
  
    void tick() override {
        handle_keyboard(runtime->logic_clock.get_delta());
        handle_mouse();
    }
};
