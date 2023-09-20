#include "GlobalRuntime.h"
#include "utils/GLContext.h"
#include "assets/Assets.h"

std::unique_ptr<GlobalRuntime> runtime;

static void opengl_init(void) {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    }

    if (!GLEW_ARB_compatibility) {
        std::cerr << "系统不支持旧的API" << std::endl;
        exit(-1);
    }
    const char *vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    std::cout << vendorName << ": " << version << std::endl;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    // glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // 逆时针的面为正面
    glDepthFunc(GL_LESS);

    checkError();
}

// 由渲染线程执行，加载资源
void init_render_resource() {
    RenderReousce &resource = runtime->renderer.resouces;
    Mesh cube_mesh{Assets::cube_vertices.data(), Assets::cube_indices.data(), (uint32_t)Assets::cube_indices.size()};
    resource.add_mesh("cube", std::move(cube_mesh));

    Mesh platform_mesh{Assets::platform_vertices.data(), Assets::cube_indices.data(),
                       (uint32_t)Assets::cube_indices.size()};

    resource.add_mesh("platform", std::move(platform_mesh));

    Mesh default_mesh{{}, {}, 0};

    resource.add_mesh("default", std::move(default_mesh));

    PhongMaterial default_material{{1.0f, 1.0f, 1.0f}, 15.0f};
    resource.add_material("default", std::move(default_material));

    Mesh line_mesh{Assets::line_vertices.data(), Assets::line_indices.data(), (uint32_t)Assets::line_indices.size()};
    resource.add_mesh("line", std::move(line_mesh));

    Mesh plane_mesh{Assets::plane_vertices.data(), Assets::plane_indices.data(),
                    (uint32_t)Assets::plane_indices.size()};
    resource.add_mesh("plane", std::move(plane_mesh));

    // 生产circle的顶点
    std::vector<Vertex> circle_vertices(101);
    // 中心点为{0, 0, 0}，半径为1，100边型，101个顶点
    circle_vertices[0] = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}; // 中心点
    for (uint32_t i = 1; i < 101; i++) {
        float angle = to_radian(360.0f / 100 * (i - 1));
        circle_vertices[i] = {{sinf(angle), cosf(angle), 0.0f},
                              {sinf(angle) / 2 + 1, cosf(angle) / 2 + 1, float(i) / 100}};
    }

    std::vector<unsigned int> circle_indices(300); // 100个三角形
    for (uint32_t i = 0; i <= 99; i++) {             // 前99个
        // 逆时针
        circle_indices[i * 3] = 0;
        circle_indices[i * 3 + 1] = i + 2;
        circle_indices[i * 3 + 2] = i + 1;
    }
    // 最后一个
    circle_indices[297] = 0;
    circle_indices[298] = 1;
    circle_indices[299] = 100;

    Mesh circle_mesh{circle_vertices.data(), circle_indices.data(), (uint32_t)circle_indices.size()};

    struct MeshResouce : ResouceItem {
        std::vector<Vertex> circle_vertices;
        std::vector<unsigned int> circle_indices;
    } stroage;
    stroage.circle_vertices = std::move(circle_vertices);
    stroage.circle_indices = std::move(circle_indices);

    resource.add_raw_resource(std::move(stroage));

    resource.add_mesh("circle", std::move(circle_mesh));
}


// 渲染线程执行的代码
DWORD WINAPI render_thread_func(LPVOID lpParam) {
    {
        OpenglContext context(runtime->display.get_hwnd());

        opengl_init();

        std::cerr << "开始初始化资源\n";
        init_render_resource();

        std::cerr << "渲染循环开始\n";
        runtime->render_clock.update();
        while (!runtime->renderer.render_thread_should_exit) {
            runtime->render_clock.update();

            runtime->renderer.render();

            runtime->fps = calculate_fps( runtime->render_clock.get_delta());
            context.swap();
        }
    }

    return 0;
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
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE: {
        float xPos = (float)LOWORD(lParam);
        float yPos = (float)HIWORD(lParam);
        // debug_log("Mouse: %f ,%f\n", xPos, yPos);
        runtime->input.handle_mouse_move(xPos, yPos, wParam);
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
        runtime->menu.display_pop_menu(hWnd, LOWORD(lParam), HIWORD(lParam));
        return 0;
    }

    case WM_COMMAND: {
        if (HIWORD(wParam) == 0) {
            // 来自目录点击的信息
            runtime->menu.on_click(LOWORD(wParam));
        }
        break;
    }

    case WM_CLOSE: {
        runtime->terminal();
        return 0;
    }

    case WM_SIZE: {
        runtime->world.window_width = LOWORD(lParam);
        runtime->world.window_height = HIWORD(lParam);
        runtime->renderer.set_target_viewport(0, 0, LOWORD(lParam), HIWORD(lParam));
        break;
    }

    default:
        break;
    }
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

void GlobalRuntime::tick() {
    logic_clock.update();
    if (!the_world_enable) {
        // std::cerr << "start tick: " << tick_count << std::endl;
        tick_count++;
        
        for (size_t i = 0; i < system_list.size(); i++) {
            system_list[i]->tick();
            if (system_list[i]->delete_me()) {
                std::swap(system_list[i], system_list.back());
                system_list.pop_back();
            }
        }
        world.tick();

        input.clear_mouse_move();
    }

    std::wstringstream formatter;
    formatter << MY_TITLE << "  ";
    formatter << L"FPS: " << fps.load() << "  ";
    formatter << L"tick: " << tick_count;
    display.set_title(formatter.str().c_str());

    // 控制执行频率
    DWORD sleep_time = (DWORD)std::max<float>(0, TIME_PERTICK - logic_clock.get_current_delta());
    Sleep(sleep_time);
}

void engine_init() {
    runtime = std::make_unique<GlobalRuntime>();
    runtime->display.show(); // 必须在runtime初始化完成后再执行

    runtime->renderer.start_thread(); // 启动渲染线程
}

void engine_shutdown() {
    runtime->system_list.clear();
    runtime->renderer.terminal_thread();
    
    runtime.reset();
}

bool GlobalRuntime::pull_events() {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) {
            return true;
        }
    }
    return false;
}
