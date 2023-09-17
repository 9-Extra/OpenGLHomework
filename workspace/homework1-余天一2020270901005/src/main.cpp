#include "engine/GlobalRuntime.h"
#include <PainterSystem.h>
#include <sstream>


void handle_mouse() {
    // 使用鼠标中键旋转视角
    //  左上角为(0,0)，右下角为(w,h)
    //  InputHandler &input = runtime->input;
    //  static bool is_middle_button_down = false;

    // if (!is_middle_button_down && input.is_middle_button_down()){
    //     runtime->display.grab_mouse();
    // }
    // if (is_middle_button_down && !input.is_middle_button_down()){
    //     runtime->display.release_mouse();
    // }

    // if (input.is_middle_button_down()) {
    //     auto [dx, dy] = input.get_mouse_move().v;
    //     //鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
    //     const float rotate_speed = 0.003f;
    //     CameraDesc &desc = runtime->world.set_camera();
    //     desc.rotation.z += dx * rotate_speed;
    //     desc.rotation.y -= dy * rotate_speed;
    // }

    // is_middle_button_down = input.is_middle_button_down();
}

void handle_keyboard(float delta) {
    // wasd移动
    //  World &world = runtime->world;
    //  const float move_speed = 0.01f * delta;

    // InputHandler &input = runtime->input;
    // if (input.is_keydown(VK_ESCAPE)) {
    //     runtime->terminal();
    // }
    // if (input.is_keydown('W')) {
    //     Vector3f ori = world.get_camera_oritation();
    //     ori.y = 0.0;
    //     world.set_camera().position += ori.normalize() * move_speed;
    // }
    // if (input.is_keydown('S')) {
    //     Vector3f ori = world.get_camera_oritation();
    //     ori.y = 0.0;
    //     world.set_camera().position += ori.normalize() * -move_speed;
    // }
    // if (input.is_keydown('A')) {
    //     Vector3f ori = world.get_camera_oritation();
    //     ori = {ori.z, 0.0, -ori.x};
    //     world.set_camera().position += ori.normalize() * move_speed;
    // }
    // if (input.is_keydown('D')) {
    //     Vector3f ori = world.get_camera_oritation();
    //     ori = {ori.z, 0.0, -ori.x};
    //     world.set_camera().position += ori.normalize() * -move_speed;
    // }
    // if (input.is_keydown(VK_SPACE)) {
    //     world.set_camera().position.y += move_speed;
    // }
    // if (input.is_keydown(VK_SHIFT)) {
    //     world.set_camera().position.y -= move_speed;
    // }
    // if (input.is_keydown('1')) {
    //     runtime->render_frame = !runtime->render_frame;
    // }
    // if (input.is_keydown('0')) {
    //     world.set_camera().position = {0.0, 0.0, 0.0};
    //     world.set_camera().rotation = {0.0, 0.0, 0.0};
    // }
}

bool event_loop() {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) {
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv) {
    try {
        
        engine_init();
        
        runtime->register_system(new PainterSystem());

        runtime->logic_clock.update();

        while (!event_loop()) {
            runtime->tick();
        }

        engine_shutdown();

        std::cout << "Normal exit!\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
