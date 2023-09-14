#include "Display.h"
#include "InputHandler.h"
#include "YGraphics.h"
#include "clock.h"
#include "models.h"
#include "render.h"
#include <iostream>
#include <memory>
#include <sstream>
#include <atomic>


class GlobalRuntime {
public:
    bool render_frame = false;

    bool the_world_enable = false;

    std::atomic_int32_t fps;

    DWORD main_thread_id;
    uint32_t window_width;
    uint32_t window_height;

    Scene scene;
    Clock logic_clock, render_clock;
    Display display;
    InputHandler input;
    Renderer renderer;

    GlobalRuntime(uint32_t width = 1080, uint32_t height = 720)
        : main_thread_id(GetCurrentThreadId()), window_width(width),
          window_height(height), display(width, height) {
        GraphicsObject cube =
            GraphicsObject(std::move(cube_vertices), std::move(cube_indices));
        cube.position = {0.0, 0.0, -10.0};
        scene.objects.emplace_back(std::move(cube));

        GraphicsObject platform = GraphicsObject(std::move(platform_vertices),
                                                 std::move(cube_indices));
        platform.scale = {4.0, 0.1, 4.0};
        platform.position = {0.0, -2.0, -10.0};
        scene.objects.emplace_back(std::move(platform));

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

std::unique_ptr<GlobalRuntime> runtime;

// 右键菜单
void handle_context_menu(HWND hWnd, LPARAM lParam) {
    HMENU hPopup = CreatePopupMenu();
    static int flag = 0;

    AppendMenu(hPopup, MF_STRING | (flag ? MF_CHECKED : 0), 1001, "选择");
    AppendMenu(hPopup, MF_SEPARATOR, 0, "");
    AppendMenu(hPopup, MF_STRING, 1002, "右键2");

    switch (TrackPopupMenu(hPopup, TPM_RETURNCMD, LOWORD(lParam),
                           HIWORD(lParam), 0, hWnd, NULL)) {
    case 1001:
        flag = !flag;
        break;
    case 1002:
        MessageBox(hWnd, "右键2", "信息", MB_OK);
        break;
    }
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    if (!runtime){
        return DefWindowProcW(hWnd, Msg, wParam, lParam);//未初始化或者结束后按默认规则处理
    }
    switch (Msg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE: {
        float xPos = (float)LOWORD(lParam);
        float yPos = (float)HIWORD(lParam);
        // debug_log("Mouse: %f ,%f\n", xPos, yPos);
        bool l_button = wParam & MK_LBUTTON;
        bool r_button = wParam & MK_RBUTTON;
        runtime->input.handle_mouse_move(xPos, yPos, l_button, r_button);
        break;
    }

    case WM_KEYDOWN: {
        runtime->input.key_down(wParam);
        return 0;
    }

    case WM_KEYUP: {
        runtime->input.key_up(wParam);
        return 0;
    }

    case WM_KILLFOCUS: {
        runtime->input.clear_keyboard_state();
        runtime->input.clear_mouse_state();
        break;
    }

    case WM_ENTERMENULOOP: {
        runtime->the_world();
        break;
    }

    case WM_EXITMENULOOP: {
        runtime->continue_run();
        break;
    }

    case WM_SETFOCUS: {
        break;
    }

    case WM_CONTEXTMENU: {
        handle_context_menu(hWnd, lParam);
        return 0;
    }

    case WM_CLOSE: {
        runtime->terminal();
        return 0;
    }

    case WM_SIZE: {
        runtime->window_width = LOWORD(lParam);
        runtime->window_height = HIWORD(lParam);
        break;
    }

    default:
        break;
    }
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

void opengl_init(void) {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    }

    if (!GLEW_ARB_compatibility) {
        std::cerr << "系统不支持旧的API" << std::endl;
        exit(-1);
    }
    const char *vendorName =
        reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *version =
        reinterpret_cast<const char *>(glGetString(GL_VERSION));
    std::cout << vendorName << ": " << version << std::endl;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    // glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);

    checkError();
}

// 只在鼠标按下时启用
void handle_mouse() {
    // 左上角为(0,0)，右下角为(w,h)
    InputHandler &input = runtime->input;
    // 处理拖拽

    if (input.is_left_button_down()) {
        auto [dx, dy] = input.get_mouse_move().v;
        // 旋转图形
        const float s = 0.003f;
        runtime->scene.objects[0].rotation += {0.0, dy * s, -dx * s};
    }

    if (input.is_right_button_down()) {
        auto [dx, dy] = input.get_mouse_move().v;
        // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
        const float rotate_speed = 0.003f;
        runtime->scene.camera.rotation.z += dx * rotate_speed;
        runtime->scene.camera.rotation.y -= dy * rotate_speed;
    }
}

void handle_keyboard(float delta) {
    Scene &scene = runtime->scene;
    const float move_speed = 0.01f * delta;

    InputHandler &input = runtime->input;
    if (input.is_keydown(VK_ESCAPE)) {
        runtime->terminal();
    }
    if (input.is_keydown('W')) {
        Vector3f ori = scene.camera.get_orientation();
        ori.y = 0.0;
        scene.camera.position += ori.normalize() * move_speed;
    }
    if (input.is_keydown('S')) {
        Vector3f ori = scene.camera.get_orientation();
        ori.y = 0.0;
        scene.camera.position += ori.normalize() * -move_speed;
    }
    if (input.is_keydown('A')) {
        Vector3f ori = scene.camera.get_orientation();
        ori = {ori.z, 0.0, -ori.x};
        scene.camera.position += ori.normalize() * move_speed;
    }
    if (input.is_keydown('D')) {
        Vector3f ori = scene.camera.get_orientation();
        ori = {ori.z, 0.0, -ori.x};
        scene.camera.position += ori.normalize() * -move_speed;
    }
    if (input.is_keydown(VK_SPACE)) {
        scene.camera.position.y += move_speed;
    }
    if (input.is_keydown(VK_SHIFT)) {
        scene.camera.position.y -= move_speed;
    }
    if (input.is_keydown('1')) {
        runtime->render_frame = !runtime->render_frame;
    }
    if (input.is_keydown('0')) {
        scene.camera.position = {0.0, 0.0, 0.0};
        scene.camera.rotation = {0.0, 0.0, 0.0};
        scene.objects[0].rotation = {0.0, 0.0, 0.0};
    }
}

void tick(float delta) {
    handle_mouse();
    handle_keyboard(delta);

    runtime->input.clear_mouse_move();
}

void render_frame() {
    glViewport(0, 0, runtime->window_width, runtime->window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    runtime->scene.camera.update(float(runtime->window_width) /
                                 float(runtime->window_height));

    if (!runtime->render_frame) {
        runtime->scene.draw_polygon();
    } else {
        runtime->scene.draw_wireframe();
    }

    std::stringstream formatter;
    Vector3f position = runtime->scene.camera.position;
    Vector3f look = runtime->scene.camera.get_orientation();

    glFlush();
    checkError();
}

// 渲染线程执行的代码
void render_thread_func() {
    std::cout << "RenderThreadStart\n";

    runtime->display.bind_opengl_context();
    opengl_init();

    runtime->render_clock.update();
    while (!runtime->renderer.render_thread_should_exit) {
        float delta = runtime->render_clock.update();

        render_frame();

        runtime->fps = calculate_fps(delta);
        runtime->display.swap();
    }

    runtime->display.delete_opengl_context();

    PostThreadMessageW(runtime->main_thread_id, WM_QUIT, 0,
                       0); // 通知主线程退出消息循环
    std::cout << "RenderThreadExit\n";
}

const float TIME_PERTICK = 1000.0f / 60.0f;

int main(int argc, char **argv) {
    try {
        runtime = std::make_unique<GlobalRuntime>();
        runtime->display.show(); // 必须在runtime初始化完成后再执行

        runtime->logic_clock.update();
        while (!runtime->display.poll_events()) {
            // if (runtime->logic_clock.get_current_delta() > TIME_PERTICK && !runtime->the_world_enable){
            //     float delta = runtime->logic_clock.update();
            //     tick(delta);
            // }
            float delta = runtime->logic_clock.update();
            if (!runtime->the_world_enable){
                tick(delta);
            }

            std::wstringstream formatter;
            formatter << L"FPS: " << runtime->fps.load() << "  ";
            if (runtime->the_world_enable) {
                formatter << "(pause)";
            }
            runtime->display.set_title(formatter.str().c_str());
            DWORD sleep_time = (DWORD)std::max<float>(0, TIME_PERTICK - runtime->logic_clock.get_current_delta());
            Sleep(sleep_time);
        }
        
        runtime.reset();

        std::cout << "Normal exit!\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
