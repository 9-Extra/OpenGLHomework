#include <CGmath.h>
#include <chrono>
#include <gl/glut.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <Windows.h>

inline void checkError() {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "GL error 0x%X: %s\n", error, gluErrorString(error));
    }
}

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

    float get_delta() const { return delta; }
    float get_current_delta() const {
        using namespace std::chrono;
        return duration_cast<duration<float, std::milli>>(steady_clock::now() - now).count();
    }
};

#define IS_KEYDOWN(key) GetKeyState(key) & 0x8000

class InputHandler {
public:
	InputHandler() {
		clear_mouse_state();
	}

	inline Vector2f get_mouse_position() const {
		return mouse_position;
	}

    inline Vector2f get_mouse_move() const {
        return mouse_delta;
    }

	inline bool is_left_button_down()  const {
		return mouse_state & (1 << mouse_button_map(GLUT_LEFT_BUTTON));
	}

	inline bool is_right_button_down()  const {
		return mouse_state & (1 << mouse_button_map(GLUT_RIGHT_BUTTON));
	}

    inline bool is_middle_button_down()  const {
		return mouse_state & (1 << mouse_button_map(GLUT_MIDDLE_BUTTON));
	}

private:
    friend void handle_mouse_click(int button, int state, int x, int y);
    friend void handle_mouse_move(int x, int y);
	Vector2f mouse_position;
    Vector2f mouse_delta;
    uint32_t mouse_state;

    //这个需要每一帧清理
    friend void loop_func();
    void clear_mouse_move(){
        mouse_delta = Vector2f(0.0f, 0.0f);
    }

	void clear_mouse_state() {
		mouse_position = Vector2f(0.5f, 0.5f);
        mouse_delta = Vector2f(0.0f, 0.0f);
        mouse_state = 0;
	}

    static constexpr unsigned char mouse_button_map(int button){
        switch (button) {
            case GLUT_LEFT_BUTTON: return 0;
            case GLUT_MIDDLE_BUTTON: return 1;
            case GLUT_RIGHT_BUTTON: return 2;
        }
        return 3;
    }
} input;

void handle_mouse_move(int x, int y) {
    // LOG_DEBUG("Real mouse handle called!");
    input.mouse_delta = input.mouse_delta + Vector2f(x, y) - input.mouse_position;
    input.mouse_position = Vector2f(x, y);
}
void handle_mouse_click(int button, int state, int x, int y) {
    // LOG_DEBUG("Real mouse handle called!");
    handle_mouse_move(x, y);

    if (state == GLUT_DOWN) {
        input.mouse_state |= 1 << InputHandler::mouse_button_map(button);
    } else {
        input.mouse_state &= 0 << InputHandler::mouse_button_map(button);
    }
}

struct Transform {
    Vector3f position;
    Vector3f rotation;
    Vector3f scale;

    Matrix transform_matrix() const {
        return Matrix::translate(position) * Matrix::rotate(rotation) * Matrix::scale(scale);
    }
};

struct Vertex {
    Vector3f position;
    Color color;
};

struct RenderItem final {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t topology;

    Matrix model_matrix;
};

struct PointLight {
    Vector3f position;
    Vector3f flux;
};

struct DirectionalLight {
    Vector3f direction;
    Vector3f flux;
};

struct Mesh {
    const Vertex *vertices;
    const uint32_t *indices;
    const uint32_t indices_count;
};

struct PhongMaterial {
    Vector3f diffusion;
    float specular_factor;
};

struct ResouceItem {
    virtual ~ResouceItem() = default;
};
class RenderReousce {
public:
    uint32_t find_material(const std::string &key) {
        if (auto it = material_look_up.find(key); it != material_look_up.end()) {
            return it->second;
        } else {
            std::cerr << "找不到material: " << key;
            exit(-1);
        }
    }

    void add_material(const std::string &key, PhongMaterial &&material) {
        uint32_t id = material_container.size();
        material_container.emplace_back(material);
        material_look_up[key] = id;
    }

    uint32_t find_mesh(const std::string &key) {
        if (auto it = mesh_look_up.find(key); it != mesh_look_up.end()) {
            return it->second;
        } else {
            std::cerr << "找不到mesh: " << key;
            exit(-1);
        }
    }

    void add_mesh(const std::string &key, Mesh &&mesh) {
        uint32_t id = mesh_container.size();
        mesh_container.emplace_back(mesh);
        mesh_look_up[key] = id;
    }

    const Mesh &get_mesh(uint32_t id) { return mesh_container[id]; }

    const PhongMaterial &get_material(uint32_t id) { return material_container[id]; }

    // 保存在退出时释放
    template <class T> void add_raw_resource(T &&item) { pool.emplace_back(std::make_unique<T>(std::move(item))); }

private:
    std::unordered_map<std::string, uint32_t> mesh_look_up;
    std::vector<Mesh> mesh_container;

    std::unordered_map<std::string, uint32_t> material_look_up;
    std::vector<PhongMaterial> material_container;

    std::vector<std::unique_ptr<ResouceItem>> pool; // 保存在这里以析构释放资源
} resources;

struct GameObjectPart {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t topology = GL_TRIANGLES;
    Matrix model_matrix = Matrix::identity();

    GameObjectPart(const std::string& mesh_name, const std::string& material_name, const uint32_t topology = GL_TRIANGLES, const Matrix model_matrix = Matrix::identity())
    : mesh_id(resources.find_mesh(mesh_name)), material_id(resources.find_material(material_name)), topology(topology), model_matrix(model_matrix) 
    {}

private:
    friend class GObject;
    Matrix base_transform;
};

struct GObjectDesc {
    Transform transform;
    std::vector<GameObjectPart> parts;
};

class GObject : public std::enable_shared_from_this<GObject> {
public:
    Transform transform;

    Matrix relate_model_matrix;
    bool is_relat_dirty = true;

    GObject(){};
    GObject(GObjectDesc &&desc) : transform(desc.transform), parts(std::move(desc.parts)){};

    // 可以重载
    virtual void tick(){
        if (is_relat_dirty){
            relate_model_matrix = transform.transform_matrix();
            for(GameObjectPart& p : parts){
                p.base_transform = relate_model_matrix * p.model_matrix;
            }
            is_relat_dirty = false;
        }
    };

    virtual void render();

private:
    friend class World;
    std::vector<GameObjectPart> parts;
};

struct Camera {
public:
    Camera() {}

    Vector3f position;
    Vector3f rotation; // zxy
    float aspect;
    float fov, near_z, far_z;

    // 获取目视方向
    Vector3f get_orientation() const {
        float pitch = rotation[1];
        float yaw = rotation[2];
        return {sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(pitch) * cosf(yaw)};
    }

    Matrix get_view_perspective_matrix() const {
        return compute_perspective_matrix(aspect, fov, near_z, far_z) * Matrix::rotate(rotation).transpose() *
               Matrix::translate(-position);
    }
};

class ISystem {
public:
    bool enable;

    ISystem(const std::string& name, bool enable=true): enable(enable), name(name){}
    const std::string& get_name() const{
        return name;
    }

    virtual void tick() = 0;

    virtual ~ISystem() = default;
private:
    const std::string name;
};

class World {
public:
    Camera camera;
    Clock clock;

    template <class T = GObject, class... ARGS> std::shared_ptr<T> create_object(ARGS &&...args) {
        std::shared_ptr<T> g_object = std::make_shared<T>(std::forward<ARGS>(args)...);
        assert(dynamic_cast<GObject *>(g_object.get()));
        objects.emplace(g_object.get(), g_object);
        return g_object;
    }

    void register_object(std::shared_ptr<GObject> g_object) {
        assert(objects.find(g_object.get()) == objects.end()); // 不可重复添加
        objects.emplace(g_object.get(), g_object);
    }

    void kill_object(std::shared_ptr<GObject> g_object) {
        size_t i = objects.erase(g_object.get());
        assert(i != 0); // 试图删除不存在的object
    }

    void register_system(ISystem* system) {
        assert(system != nullptr && systems.find(system->get_name()) == systems.end());
        systems.emplace(system->get_name(), system);
    }

    ISystem* get_system(const std::string& name){
        return systems.at(name).get();
    }

    void remove_system(const std::string& name){
        systems.erase(name);
    }

    void tick();

    void render();
    // 获取屏幕上的一点对应的射线方向
    Vector3f get_screen_point_oritation(Vector2f screen_xy) const;

private:
    std::unordered_map<GObject *, std::shared_ptr<GObject>> objects;
    std::unordered_map<std::string, std::unique_ptr<ISystem>> systems;
};

struct RenderInfo {
    struct Viewport {
        GLint x;
        GLint y;
        GLsizei width;
        GLsizei height;
    } main_viewport;
};

World world;
RenderInfo render_info;

void set_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    render_info.main_viewport = {x, y, width, height};
    world.camera.aspect = float(width) / float(height);
}

void setup_opengl() {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    // glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // 逆时针的面为正面
    glDepthFunc(GL_LESS);

    checkError();
}

// render的默认实现
void GObject::render() {
    {
        for (const GameObjectPart &p : parts) {
            const Mesh &mesh = resources.get_mesh(p.mesh_id);
            const PhongMaterial &material = resources.get_material(p.material_id);

            // glMaterialfv(GL_FRONT, GL_DIFFUSE, material.diffusion.data());

            glLoadMatrixf(p.base_transform.transpose().data());
            glBegin(p.topology);
            for (uint32_t i = 0; i < mesh.indices_count; i++) {
                const Vertex &v = mesh.vertices[mesh.indices[i]];
                glColor3fv(v.color.data());
                glVertex3fv(v.position.data());
            }
            glEnd();
        }
    }
}

namespace Assets {
const std::vector<Vertex> cube_vertices = {{{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}}, {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                                           {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},   {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},
                                           {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},  {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
                                           {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},    {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}}};

const std::vector<Vertex> platform_vertices = {
    {{-1.0, -1.0, -1.0}, {0.5, 0.5, 0.5}}, {{1.0, -1.0, -1.0}, {0.5, 0.5, 0.5}}, {{1.0, 1.0, -1.0}, {0.5, 0.5, 0.5}},
    {{-1.0, 1.0, -1.0}, {0.5, 0.5, 0.5}},  {{-1.0, -1.0, 1.0}, {0.5, 0.5, 0.5}}, {{1.0, -1.0, 1.0}, {0.5, 0.5, 0.5}},
    {{1.0, 1.0, 1.0}, {0.5, 0.5, 0.5}},    {{-1.0, 1.0, 1.0}, {0.5, 0.5, 0.5}}};

const std::vector<unsigned int> cube_indices = {1, 0, 3, 3, 2, 1, 3, 7, 6, 6, 2, 3, 7, 3, 0, 0, 4, 7,
                                                2, 6, 5, 5, 1, 2, 4, 5, 6, 6, 7, 4, 5, 4, 0, 0, 1, 5};

const std::vector<Vertex> plane_vertices = {
    {{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<unsigned int> plane_indices = {2, 1, 0, 3, 2, 0};

const std::vector<Vertex> line_vertices = {{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                                           {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}};

const std::vector<unsigned int> line_indices = {0, 1};
} // namespace Assets

void init_resource() {
    RenderReousce &resource = resources;
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
    for (size_t i = 1; i < 101; i++) {
        float angle = to_radian(360.0f / 100 * (i - 1));
        circle_vertices[i] = {{sinf(angle), cosf(angle), 0.0f},
                              {sinf(angle) / 2 + 1, cosf(angle) / 2 + 1, float(i) / 100}};
    }

    std::vector<unsigned int> circle_indices(300); // 100个三角形
    for (size_t i = 0; i <= 99; i++) {             // 前99个
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

void init_start_scene() {
    {
        Camera &desc = world.camera;
        desc.position = {0.0f, 0.0f, 0.0f};
        desc.rotation = {0.0f, 0.0f, 0.0f};
        desc.fov = 1.57f;
        desc.near_z = 1.0f;
        desc.far_z = 1000.0f;
        desc.aspect = float(render_info.main_viewport.width) / float(render_info.main_viewport.height);
    }

    {
        world.create_object(GObjectDesc{{{0.0f, 0.0f, -10.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                                        {{"cube", "default"}}});
    }
    // {
    //     create_object(GObjectDesc{
    //         {0.0f, 10.0f, -10.0f},
    //         {0.0f, 0.0f, 0.0f},
    //         {5.0f, 5.0f, 5.0f},
    //         {GameObjectPartDesc{"plane", "default"}}
    //     });
    // }
    // {
    //     GObject &platform = this->objects.at(this->create_object());
    //     platform.set_parts().emplace_back(
    //         GameObjectPartDesc{"platform", "default"});
    //     platform.set_position({0.0, -2.0, -10.0});
    //     platform.set_scale({4.0, 0.1, 4.0});
    // }
}

inline unsigned int calculate_fps(float delta_time) {
    const float ratio = 0.1f;
    static float avarage_frame_time = std::nan("");

    if (std::isnormal(avarage_frame_time)) {
        avarage_frame_time = avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time);
}

void World::tick() {
    clock.update();

    for(const auto& [name, sys] : systems){
        sys->tick();
    }

    for (auto &[ptr, obj] : objects) {
        obj->tick();
    }
}

void World::render() {
    glDrawBuffer(GL_BACK); // 渲染到后缓冲区
    RenderInfo::Viewport &v = render_info.main_viewport;
    glViewport(v.x, v.y, v.width, v.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera.get_view_perspective_matrix().transpose().data());

    glMatrixMode(GL_MODELVIEW);

    for (auto &[ptr, obj] : objects) {
        obj->render();
    }

    glutSwapBuffers();
    checkError();
}

#define MY_TITLE "2020270901005 homework 3"

void loop_func() {
    world.tick();
    world.render();

    input.clear_mouse_move();
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
        static bool is_middle_button_down = false;

        // if (!is_middle_button_down && input.is_middle_button_down()) {
        //     runtime->display.grab_mouse();
        // }
        // if (is_middle_button_down && !input.is_middle_button_down()) {
        //     runtime->display.release_mouse();
        // }

        if (input.is_middle_button_down()) {
            auto [dx, dy] = input.get_mouse_move().v;
            // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
            const float rotate_speed = 0.003f;
            Camera &desc = world.camera;
            desc.rotation.z += dx * rotate_speed;
            desc.rotation.y -= dy * rotate_speed;
        }

        is_middle_button_down = input.is_middle_button_down();
    }

    void handle_keyboard(float delta) {
        // wasd移动
        const float move_speed = 0.02f * delta;

        if (IS_KEYDOWN(VK_ESCAPE)) {
            PostQuitMessage(0);
        }
        if (IS_KEYDOWN('W')) {
            Vector3f ori = world.camera.get_orientation();
            ori.y = 0.0;
            world.camera.position += ori.normalize() * move_speed;
        }
        if (IS_KEYDOWN('S')) {
            Vector3f ori = world.camera.get_orientation();
            ori.y = 0.0;
            world.camera.position += ori.normalize() * -move_speed;
        }
        if (IS_KEYDOWN('A')) {
            Vector3f ori = world.camera.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            world.camera.position += ori.normalize() * move_speed;
        }
        if (IS_KEYDOWN('D')) {
            Vector3f ori = world.camera.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            world.camera.position += ori.normalize() * -move_speed;
        }
        if (IS_KEYDOWN(VK_SPACE)) {
            world.camera.position.y += move_speed;
        }
        if (IS_KEYDOWN(VK_SHIFT)) {
            world.camera.position.y -= move_speed;
        }

        if (IS_KEYDOWN('0')) {
            world.camera.position = {0.0, 0.0, 0.0};
            world.camera.rotation = {0.0, 0.0, 0.0};
        }
    }
  
    void tick() override {
        handle_keyboard(world.clock.get_delta());
        handle_mouse();
    }
};

int main(int argc, char **argv) {
    std::cout << "Hello\n";

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(50, 100);
    glutInitWindowSize(400, 300);
    glutCreateWindow(MY_TITLE);

    set_viewport(0, 0, 400, 300);

    glutIdleFunc(loop_func);
    glutDisplayFunc([]() {});
    glutReshapeFunc([] (int w, int h){
        set_viewport(0, 0, w, h);
    });
    glutMouseFunc(handle_mouse_click);
    glutMotionFunc(handle_mouse_move);
    glutPassiveMotionFunc(handle_mouse_move);

    setup_opengl();
    init_resource();
    init_start_scene();

    world.register_system(new MoveSystem("move"));

    world.clock.update();

    glutMainLoop();
    return 0;
}