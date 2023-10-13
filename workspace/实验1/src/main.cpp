#include <GL/glew.h>
#include <GL/glut.h>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "winapi.h"
#include "CGmath.h"
#include "Sjson.h"
#include "RenderResource.h"
#include "utils.h"
#include "GObject.h"
#include "Renderer.h"
#include "World.h"
#include "HardcodeAssets.h"
#include "Simgui.h"
#include "CpntMeshRender.h"

#include <FreeImage.h>

#define INIT_WINDOW_WIDTH 1080
#define INIT_WINDOW_HEIGHT 720
#define MY_TITLE "2020270901005 homework 3"

// Mingw的ifstream不知道为什么导致了崩溃，手动实现文件读取
std::string read_whole_file(const std::string &path) {
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file: " << path << std::endl;
        exit(-1);
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size)) {
        std::cerr << "Failed to get file size\n";
        exit(-1);
    }
    std::string chunk;
    chunk.resize(size.QuadPart);

    char *ptr = (char *)chunk.data();
    char *end = ptr + chunk.size();
    DWORD read = 0;
    while (ptr < end) {
        DWORD to_read = (DWORD)std::min<size_t>(std::numeric_limits<DWORD>::max(), end - ptr);
        if (!ReadFile(handle, ptr, to_read, &read, NULL)) {
            std::cerr << "Failed to read file\n";
            exit(-1);
        }
        ptr += read;
    }

    CloseHandle(handle);

    return chunk;
}

#define IS_KEYDOWN(key) GetKeyState(key) & 0x8000

class InputHandler {
public:
    InputHandler() { clear_mouse_state(); }

    inline Vector2f get_mouse_position() const { return mouse_position; }
    inline Vector2f get_mouse_move() const { return mouse_delta; }
    inline bool is_left_button_down() const { return mouse_state & (1 << mouse_button_map(GLUT_LEFT_BUTTON)); }
    inline bool is_right_button_down() const { return mouse_state & (1 << mouse_button_map(GLUT_RIGHT_BUTTON)); }
    inline bool is_middle_button_down() const { return mouse_state & (1 << mouse_button_map(GLUT_MIDDLE_BUTTON)); }

private:
    friend void handle_mouse_click(int button, int state, int x, int y);
    friend void handle_mouse_move(int x, int y);
    Vector2f mouse_position;
    Vector2f mouse_delta;
    uint32_t mouse_state;

    // 这个需要每一帧清理
    friend void loop_func();
    void clear_mouse_move() { mouse_delta = Vector2f(0.0f, 0.0f); }
    void clear_mouse_state() {
        mouse_position = Vector2f(0.5f, 0.5f);
        mouse_delta = Vector2f(0.0f, 0.0f);
        mouse_state = 0;
    }
    static constexpr unsigned char mouse_button_map(int button) {
        switch (button) {
        case GLUT_LEFT_BUTTON:
            return 0;
        case GLUT_MIDDLE_BUTTON:
            return 1;
        case GLUT_RIGHT_BUTTON:
            return 2;
        }
        return 3;
    }
} input;

void handle_mouse_move(int x, int y) {
    SimGui::update_mouse_pos(x, y);
    input.mouse_delta = input.mouse_delta + Vector2f(x, y) - input.mouse_position;
    input.mouse_position = Vector2f(x, y);
}
void handle_mouse_click(int button, int state, int x, int y) {
    handle_mouse_move(x, y);
    if (state == GLUT_DOWN) {
        if (button == GLUT_LEFT_BUTTON) {
            SimGui::do_mouse_down();
        }
        input.mouse_state |= 1 << InputHandler::mouse_button_map(button);
    } else {
        if (button == GLUT_LEFT_BUTTON) {
            SimGui::do_mouse_release();
        }
        input.mouse_state &= 0 << InputHandler::mouse_button_map(button);
    }
}

void setup_opengl() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    }

    const char *vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    std::cout << vendorName << ": " << version << std::endl;

    if (!GL_EXT_gpu_shader4) {
        std::cerr << "不兼容拓展" << std::endl;
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // 逆时针的面为正面
    glDepthFunc(GL_LEQUAL); // 在z = 1.0时，深度测试结果为true

    checkError();
}

void init_resource() {
    // 从json加载大部分的资源
    resources.load_json(R"(D:\programming\OpenGL开发包\workspace\实验1\assets\resources.json)");
    // 材质资源比较特殊，暂时通过硬编码加载
    {
        // 单一颜色材质
        MaterialDesc green_material_desc;
        green_material_desc.shader_name = "single_color";
        resources.add_material("default", green_material_desc);
    }
    // 部分硬编码的mesh
    resources.add_mesh("default", {}, {});
    resources.add_mesh("plane", Assets::plane_vertices, Assets::plane_indices);
    resources.add_mesh("triangle", Assets::triange_vertices, Assets::triange_indices);
}


void init_start_scene() {
    load_scene_from_json(R"(D:\programming\OpenGL开发包\workspace\实验1\assets\scene1.json)"); // 整个场景的所有物体都从json加载了
}

unsigned int calculate_fps(float delta_time) {
    const float ratio = 0.1f; // 平滑比例
    static float avarage_frame_time = std::nan("");

    if (std::isnormal(avarage_frame_time)) {
        avarage_frame_time = avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time);
}

void loop_func() {
    
    world.tick();
    renderer.render();

    input.clear_mouse_move();

    // 计算和显示FPS
    std::stringstream formatter;
    formatter << MY_TITLE << "  ";
    formatter << "FPS: " << calculate_fps(world.clock.get_delta()) << "  ";
    glutSetWindowTitle(formatter.str().c_str());
}

class MoveSystem : public ISystem {
private:
public:
    using ISystem::ISystem;
    void handle_mouse() {
        // 使用鼠标中键旋转视角
        //  左上角为(0,0)，右下角为(w,h)
        static bool is_left_button_down = false;
        static bool is_right_button_down = false;

        if (!is_left_button_down && input.is_left_button_down()) {
        }

        if (!is_right_button_down && input.is_right_button_down()) {
        }

        if (input.is_middle_button_down()) {
            auto [dx, dy] = input.get_mouse_move().v;
            // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
            const float rotate_speed = 0.003f;
            Vector3f& rotation = camera->transform.rotation;
            rotation.z += dx * rotate_speed;
            rotation.y -= dy * rotate_speed;
            camera->is_relat_dirty = true;
        }

        is_left_button_down = input.is_left_button_down();
        is_right_button_down = input.is_right_button_down();
    }

    void handle_keyboard(float delta) {
        // wasd移动
        const float move_speed = 0.02f * delta;

        if (IS_KEYDOWN(VK_ESCAPE)) {
            PostQuitMessage(0);
        }
        if (IS_KEYDOWN('W')) {
            Vector3f ori = camera->transform.get_orientation();
            ori.y = 0.0;
            camera->transform.position += ori.normalize() * move_speed;
            camera->is_relat_dirty = true;
        }
        if (IS_KEYDOWN('S')) {
            Vector3f ori = camera->transform.get_orientation();
            ori.y = 0.0;
            camera->transform.position += ori.normalize() * -move_speed;
            camera->is_relat_dirty = true;
        }
        if (IS_KEYDOWN('A')) {
            Vector3f ori = camera->transform.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            camera->transform.position += ori.normalize() * move_speed;
            camera->is_relat_dirty = true;
        }
        if (IS_KEYDOWN('D')) {
            Vector3f ori = camera->transform.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            camera->transform.position += ori.normalize() * -move_speed;
            camera->is_relat_dirty = true;
        }
        if (IS_KEYDOWN(VK_SPACE)) {
            camera->transform.position.y += move_speed;
            camera->is_relat_dirty = true;
        }
        if (IS_KEYDOWN(VK_SHIFT)) {
            camera->transform.position.y -= move_speed;
            camera->is_relat_dirty = true;
        }

        if (IS_KEYDOWN('0')) {
            camera->transform = Transform{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1, 1, 1}};
            camera->is_relat_dirty = true;
        }

        if (IS_KEYDOWN(VK_UP)) {
            renderer.fog_density += 0.00001f * delta;
        }

        if (IS_KEYDOWN(VK_DOWN)) {
            renderer.fog_density -= 0.00001f * delta;
            renderer.fog_density = std::max(renderer.fog_density, 0.0f);
        }
    }

    void on_attach() override {
        left = world.get_root()->get_child_by_name("left");
        assert(left);
        right = world.get_root()->get_child_by_name("right");
        assert(right);
        camera = world.get_root()->get_child_by_name("相机");
        assert(camera);
    }

    void tick() override {
        handle_keyboard(world.clock.get_delta());
        handle_mouse();

        //v = (std::sin(world.get_tick_count() * 0.01f) + 1) / 2;
        uint32_t w = renderer.main_viewport.width;
        uint32_t h = renderer.main_viewport.height;

        uint32_t rgb_x = uint32_t(0.3f * w);
        uint32_t rgb_y = uint32_t(0.7f * h);
        SimGui::show_text("RBG", {rgb_x, rgb_y}, "RGB");
        bool is_rgb_update = false;
        is_rgb_update |= SimGui::show_cell_float("r", {rgb_x - 50, rgb_y + 30, 40, 25}, &color_rgb.x);
        is_rgb_update |= SimGui::show_cell_float("g", {rgb_x, rgb_y + 30, 40, 25}, &color_rgb.y);
        is_rgb_update |= SimGui::show_cell_float("b", {rgb_x + 50, rgb_y + 30, 40, 25}, &color_rgb.z);
        
        uint32_t hsv_x = uint32_t(0.7f * w);
        uint32_t hsv_y = uint32_t(0.7f * h);
        SimGui::show_text("HSV", {hsv_x, hsv_y}, "HSV");
        bool is_hsv_update = false;
        is_hsv_update |= SimGui::show_cell_float("h", {hsv_x - 50, hsv_y + 30, 40, 25}, &color_hsv.x, 0.01f, 0.0f, 360.0f, "%.0f");
        is_hsv_update |= SimGui::show_cell_float("s", {hsv_x, hsv_y + 30, 40, 25}, &color_hsv.y);
        is_hsv_update |= SimGui::show_cell_float("v", {hsv_x + 50, hsv_y + 30, 50, 25}, &color_hsv.z);
        if (is_rgb_update) {
            color_hsv = rgb2hsv(color_rgb);
        } else if (is_hsv_update) {
            color_rgb = hsv2rgb(color_hsv);
        }
        
        left->get_component<CpntMeshRender>()->get_parts()[0].color = color_rgb; 
        right->get_component<CpntMeshRender>()->get_parts()[0].color = color_rgb; 
    }

private:
    Vector3f color_rgb = Vector3f(0.0f, 0.0f, 0.0);
    Vector3f color_hsv = rgb2hsv(color_rgb);

    std::shared_ptr<GObject> camera;
    std::shared_ptr<GObject> left;
    std::shared_ptr<GObject> right;
};

int main(int argc, char **argv) {

    Vector3f r = {0.5f, 0.2f, 0.3f};
    Vector3f h = rgb2hsv(r);
    std::cout << h << std::endl;
    std::cout << hsv2rgb(h) << std::endl;
    // 初始化glut和创建窗口
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);
    glutCreateWindow(MY_TITLE);

    // 注册相关函数
    glutIdleFunc(loop_func);  // 一直不停执行循环函数，此函数包含主要逻辑
    glutDisplayFunc([]() {}); // 无视DisplayFunc
    // 保证视口与窗口大小相同
    glutReshapeFunc([](int w, int h) { 
        SimGui::set_screen_size(w, h);
        renderer.set_viewport(0, 0, w, h); 
    });
    glutMouseFunc(handle_mouse_click);
    glutMotionFunc(handle_mouse_move);
    glutPassiveMotionFunc(handle_mouse_move);

    // 初始化opengl
    setup_opengl();

    SimGui::init();

    // 初始化资源和加载资源
    init_resource();

    //初始化渲染相关pass等
    renderer.init();
    renderer.set_viewport(0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT); // 初始化时也需要设置一下视口
    SimGui::set_screen_size(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);

    // 加载初始场景
    init_start_scene();

    // 注册系统
    world.register_system(new MoveSystem("move"));

    // 注册在退出时执行的清理操作
    atexit([] {
        world.clear();
        renderer.clear();
        resources.clear();
        std::cout << "Exit!\n";
    });

    world.clock.update(); // 重置一下时钟

    // XX，启动！
    glutMainLoop();

    return 0;
}