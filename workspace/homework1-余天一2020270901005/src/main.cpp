#include "GlobalRuntime.h"
#include "assets.h"
#include <sstream>

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
    if (!runtime) {
        return DefWindowProcW(hWnd, Msg, wParam,
                              lParam); // 未初始化或者结束后按默认规则处理
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
        runtime->the_world();
        runtime->input.clear_keyboard_state();
        runtime->input.clear_mouse_state();
        break;
    }

    case WM_SETFOCUS: {
        runtime->continue_run();
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

    case WM_CONTEXTMENU: {
        handle_context_menu(hWnd, lParam);
        return 0;
    }

    case WM_CLOSE: {
        runtime->terminal();
        return 0;
    }

    case WM_SIZE: {
        CameraDesc &camera = runtime->world.set_camera();
        camera.window_width = LOWORD(lParam);
        camera.window_height = HIWORD(lParam);
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

    if (input.is_left_button_down()) {
        auto [dx, dy] = input.get_mouse_move().v;
        // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
        const float rotate_speed = 0.003f;
        CameraDesc &desc = runtime->world.set_camera();
        desc.rotation.z += dx * rotate_speed;
        desc.rotation.y -= dy * rotate_speed;
    }
}

void handle_keyboard(float delta) {
    World &world = runtime->world;
    const float move_speed = 0.01f * delta;

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
    if (input.is_keydown('1')) {
        runtime->render_frame = !runtime->render_frame;
    }
    if (input.is_keydown('0')) {
        world.set_camera().position = {0.0, 0.0, 0.0};
        world.set_camera().rotation = {0.0, 0.0, 0.0};
    }
}

const float TIME_PERTICK = 1000.0f / 60.0f;

void tick() {
    static uint32_t tick_count = 0;
    float delta = runtime->logic_clock.update();
    if (!runtime->the_world_enable) {
        //std::cerr << "start tick: " << tick_count << std::endl;
        tick_count++;
        handle_mouse();
        handle_keyboard(delta);

        runtime->input.clear_mouse_move();

        runtime->world.tick(delta);
    }

    std::wstringstream formatter;
    formatter << L"FPS: " << runtime->fps.load() << "  ";
    formatter << L"tick: " << tick_count;
    runtime->display.set_title(formatter.str().c_str());

    // 控制执行频率
    DWORD sleep_time = (DWORD)std::max<float>(
        0, TIME_PERTICK - runtime->logic_clock.get_current_delta());
    Sleep(sleep_time);
}

// 由渲染线程执行，加载资源
void init_render_resource() {
    RenderReousce &resource = runtime->renderer.resouces;
    Mesh cube_mesh{Assets::cube_vertices, Assets::cube_indices,
                   (uint32_t)Assets::cube_indices.size()};
    resource.add_mesh("cube", std::move(cube_mesh));

    Mesh platform_mesh{Assets::platform_vertices, Assets::cube_indices,
                       (uint32_t)Assets::cube_indices.size()};

    resource.add_mesh("platform", std::move(platform_mesh));

    Mesh default_mesh{{}, {}, 0};

    resource.add_mesh("default", std::move(default_mesh));

    PhongMaterial default_material{{1.0f, 1.0f, 1.0f}, 15.0f};
    resource.add_material("default", std::move(default_material));
}

// 渲染线程执行的代码
void render_thread_func() {
    std::cerr << "RenderThreadStart\n";

    runtime->display.bind_opengl_context();
    opengl_init();

    std::cerr << "开始初始化资源\n";
    init_render_resource();

    std::cerr << "渲染循环开始\n";
    runtime->render_clock.update();
    while (!runtime->renderer.render_thread_should_exit) {
        float delta = runtime->render_clock.update();

        runtime->renderer.render();

        runtime->fps = calculate_fps(delta);
        runtime->display.swap();
    }

    runtime->display.delete_opengl_context();

    PostThreadMessageW(runtime->main_thread_id, WM_QUIT, 0,
                       0); // 通知主线程退出消息循环
    std::cout << "RenderThreadExit\n";
}

bool event_loop() {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT){
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv) {
    try {
        runtime = std::make_unique<GlobalRuntime>();
        runtime->display.show(); // 必须在runtime初始化完成后再执行

        runtime->logic_clock.update();

        while (!event_loop()) {
            tick();
        }

        runtime.reset();

        std::cout << "Normal exit!\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
