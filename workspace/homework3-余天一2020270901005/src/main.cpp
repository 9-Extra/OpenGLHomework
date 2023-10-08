#include <GL/glew.h>
#include <GL/glut.h>
#include <algorithm>
#include <array>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "winapi.h"
#include "CGmath.h"
#include "Sjson.h"
#include "RenderResource.h"
#include "utils.h"

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

    float get_delta() const { return delta; } // 返回该帧相对上一帧过去的以float表示的毫秒数
    float get_current_delta() const // 获得调用此函数的时间相对上一帧过去的以float表示的毫秒数
    {
        using namespace std::chrono;
        return duration_cast<duration<float, std::milli>>(steady_clock::now() - now).count();
    }
};

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
    input.mouse_delta = input.mouse_delta + Vector2f(x, y) - input.mouse_position;
    input.mouse_position = Vector2f(x, y);
}
void handle_mouse_click(int button, int state, int x, int y) {
    handle_mouse_move(x, y);

    if (state == GLUT_DOWN) {
        input.mouse_state |= 1 << InputHandler::mouse_button_map(button);
    } else {
        input.mouse_state &= 0 << InputHandler::mouse_button_map(button);
    }
}

struct GameObjectPart {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t topology;
    Matrix model_matrix;
    Matrix normal_matrix;

    GameObjectPart(const std::string &mesh_name, const std::string &material_name,
                   const uint32_t topology = GL_TRIANGLES,
                   const Transform &transform = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}})
        : mesh_id(resources.meshes.find(mesh_name)), material_id(resources.materials.find(material_name)),
          topology(topology), model_matrix(transform.transform_matrix()), normal_matrix(transform.normal_matrix()) {}
    
    //在遍历节点树时计算和填写
    Matrix base_transform;
    Matrix base_normal_matrix;
};

struct GObjectDesc {
    Transform transform;
    std::vector<GameObjectPart> parts;
};

class GObject final : public std::enable_shared_from_this<GObject> {
public:
    std::string name;
    Transform transform;

    Matrix relate_model_matrix;
    Matrix relate_normal_matrix;
    bool is_relat_dirty = true;

    GObject(const std::string name = "") : name(name){};
    GObject(GObjectDesc &&desc, const std::string name = "")
        : name(name), transform(desc.transform), parts(std::move(desc.parts)){};

    void add_part(const GameObjectPart &part) {
        GameObjectPart &p = parts.emplace_back(part);
        p.base_transform = relate_model_matrix * p.model_matrix;
        p.base_normal_matrix = transform.normal_matrix() * p.normal_matrix;
    }

    std::vector<GameObjectPart> &get_parts() { return parts; }

    ~GObject() {
        assert(!parent.lock()); // 必须没有父节点
    };

    void set_transform(const Transform &transform) {
        this->transform = transform;
        is_relat_dirty = true;
    }

    const std::vector<std::shared_ptr<GObject>> &get_children() const { return children; }
    std::shared_ptr<GObject> get_child_by_name(const std::string name) {
        if (name.empty())
            return nullptr;
        for (auto &child : children) {
            if (child->name == name) {
                return child;
            }
        }
        return nullptr;
    }

    void attach_child(std::shared_ptr<GObject> child) {
        children.push_back(child);
        child->parent = weak_from_this();
    }

    void remove_child(GObject *child) {
        auto it =
            std::find_if(children.begin(), children.end(), [child](const auto &c) -> bool { return c.get() == child; });
        if (it != children.end()) {
            (*it)->parent.reset();
            children.erase(it);
        } else {
            assert(false); // 试图移除不存在的子节点
        }
    }

    bool has_parent() const { return !parent.expired(); }

    std::weak_ptr<GObject> get_parent() { return parent; }

    void attach_parent(GObject *new_parent) {
        if (new_parent == parent.lock().get())
            return;
        if (auto old_parent = parent.lock(); old_parent) {
            old_parent->remove_child(this);
        }
        if (new_parent != nullptr) {
            new_parent->attach_child(shared_from_this());
        }
    }

private:
    std::vector<GameObjectPart> parts;
    std::weak_ptr<GObject> parent;
    std::vector<std::shared_ptr<GObject>> children;
};

// 一个可写的uniform buffer对象的封装
template <class T> struct WritableUniformBuffer {
    // 在初始化opengl后才能初始化
    void init() {
        assert(id == 0);
        glGenBuffers(1, &id);
        glBindBuffer(GL_UNIFORM_BUFFER, id);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
    }

    T *map() {
        void *ptr = glMapNamedBuffer(id, GL_WRITE_ONLY);
        assert(ptr != nullptr);
        return (T *)ptr;
    }
    void unmap() {
        bool ret = glUnmapNamedBuffer(id);
        assert(ret);
    }

    unsigned int get_id() const { return id; }

    void clear() {
        glDeleteBuffers(1, &id);
        id = 0;
    }

private:
    unsigned int id = 0;
};

class Pass{
public:
    virtual void run() = 0;
    friend class Renderer;
};

class LambertianPass : public Pass {
public:
    LambertianPass() {
        per_frame_uniform.init();
        per_object_uniform.init();
    }

    ~LambertianPass(){
        per_frame_uniform.clear();
        per_object_uniform.clear();
    }

    void accept(GameObjectPart* part){
        parts.push_back(part);
    }

    void reset() {
        parts.clear();
    }

    virtual void run() override;

private:
    static constexpr unsigned int POINTLIGNT_MAX = 8;
    struct PointLightData final {
        alignas(16) Vector3f position;  // 0
        alignas(16) Vector3f intensity; // 4
    };
    struct PerFrameData final {
        Matrix view_perspective_matrix;
        alignas(16) Vector3f ambient_light; // 3 * 4 + 4
        alignas(16) Vector3f camera_position;
        alignas(4) float fog_min_distance;
        alignas(4) float fog_density;
        alignas(4) uint32_t pointlight_num; // 4
        PointLightData pointlight_list[POINTLIGNT_MAX];
    };

    struct PerObjectData {
        Matrix model_matrix;
        Matrix normal_matrix;
    };

    std::vector<GameObjectPart*> parts; // 记录要渲染的对象
    WritableUniformBuffer<PerFrameData> per_frame_uniform;   // 用于一般渲染每帧变化的数据
    WritableUniformBuffer<PerObjectData> per_object_uniform; // 用于一般渲染每个物体不同的数据
};

class SkyBoxPass : public Pass{
public:
    SkyBoxPass(){
        shader_program_id = resources.shaders.get(resources.shaders.find("skybox")).program_id;
        mesh_id = resources.meshes.find("skybox_cube");

        skybox_uniform.init();
    }

    ~SkyBoxPass() {
        skybox_uniform.clear();
    }
    void set_skybox(unsigned int texture_id) {
        skybox_texture_id = texture_id;
    }
    virtual void run() override;
private:
    const static unsigned int SKYBOX_TEXTURE_BINDIGN = 5;
    struct SkyBoxData final {
        Matrix skybox_view_perspective_matrix;
    };

    //每帧更新
    WritableUniformBuffer<SkyBoxData> skybox_uniform;
    unsigned int skybox_texture_id = 0;
    //初始化时设定
    unsigned int shader_program_id;
    uint32_t mesh_id;
};

class PickupPass : public Pass {
public:
    PickupPass(){
        // 初始化pickup用的framebuffer
        glGenRenderbuffers(1, &framebuffer_pickup_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, framebuffer_pickup_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // 完整的初始化推迟到使用的时候
        checkError();

        glGenFramebuffers(1, &framebuffer_pickup);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_pickup);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, framebuffer_pickup_rbo);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        checkError();
    }

    virtual void run() override;

    ~PickupPass(){
        glDeleteRenderbuffers(1, &framebuffer_pickup_rbo);
        glDeleteFramebuffers(1, &framebuffer_pickup);
    }
private:
    unsigned int framebuffer_pickup;
    unsigned int framebuffer_pickup_rbo;

    // 设置framebuffer大小，显然要和视口一样大
    void set_framebuffer_size(unsigned int width, unsigned int height) {
        glNamedRenderbufferStorage(framebuffer_pickup_rbo, GL_R32UI, width, height);
        checkError();
        assert(glCheckNamedFramebufferStatus(framebuffer_pickup, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
};

class Renderer final{
public:
    struct Viewport {
        GLint x;
        GLint y;
        GLsizei width;
        GLsizei height;
    } main_viewport; // 主视口

    // passes
    std::unique_ptr<LambertianPass> lambertian_pass;
    std::unique_ptr<SkyBoxPass> skybox_pass;
    std::unique_ptr<PickupPass> pickup_pass;

    void init() {
        lambertian_pass = std::make_unique<LambertianPass>();
        skybox_pass = std::make_unique<SkyBoxPass>();
        pickup_pass = std::make_unique<PickupPass>();
    }

    void accept(GameObjectPart* part){
        lambertian_pass->accept(part);
    }

    void render(){
        lambertian_pass->run();
        skybox_pass->run();
        //pickup_pass->run();

        lambertian_pass->reset();
        glutSwapBuffers(); // 渲染完毕，交换缓冲区，显示新一帧的画面
        checkError();
    }

    void set_viewport(GLint x, GLint y, GLsizei width, GLsizei height);

    void clear() {
        lambertian_pass.reset();
        skybox_pass.reset();
        pickup_pass.reset();
    }
} renderer;

struct Camera {
public:
    Camera() {
        position = {0.0f, 0.0f, 0.0f};
        rotation = {0.0f, 0.0f, 0.0f};
        fov = 1.57f;
        near_z = 1.0f;
        far_z = 1000.0f;
    }

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

    Vector3f get_up_direction() const {
        float sp = sinf(rotation[1]);
        float cp = cosf(rotation[1]);
        float cr = cosf(rotation[0]);
        float sr = sinf(rotation[0]);
        float sy = sinf(rotation[2]);
        float cy = cosf(rotation[2]);

        return {-sp * sy * cr - sr * cy, cp * cr, sp * cr * cy - sr * sy};
    }

    // 用于一般物体的变换矩阵
    Matrix get_view_perspective_matrix() const {
        return compute_perspective_matrix(aspect, fov, near_z, far_z) * Matrix::rotate(rotation).transpose() *
               Matrix::translate(-position);
    }
    // 用于天空盒的变换矩阵
    Matrix get_skybox_view_perspective_matrix() const {
        return compute_perspective_matrix(aspect, fov, near_z, far_z) * Matrix::rotate(rotation).transpose() *
               Matrix::scale({far_z / 2, far_z / 2, far_z / 2});
    }
};

struct SkyBox {
    unsigned int color_texture_id;
};

class ISystem {
public:
    bool enable;

    ISystem(const std::string &name, bool enable = true) : enable(enable), name(name) {}
    const std::string &get_name() const { return name; }

    virtual void on_attach(){};
    virtual void tick() = 0;

    virtual ~ISystem() = default;

private:
    const std::string name;
};

class World {
public:
    Camera camera;
    Clock clock;

    Vector3f ambient_light = {0.02f, 0.02f, 0.02f}; // 环境光
    std::vector<PointLight> pointlights;         // 点光源
    bool is_light_dirty = true;

    float fog_min_distance = 5.0f; // 雾开始的距离
    float fog_density = 0.001f;    // 雾强度

    World() {
        clear_objects();
    }

    void clear() {
        root = nullptr;
        systems.clear();
    }

    uint64_t get_tick_count() { return tick_count; }
    std::shared_ptr<GObject> get_root() { return root; }
    void clear_objects() {
        root = std::make_shared<GObject>(GObjectDesc{{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, {}}, "root");
    }

    void register_system(ISystem *system) {
        assert(system != nullptr && systems.find(system->get_name()) == systems.end());
        systems.emplace(system->get_name(), system);
        system->on_attach();
    }

    void set_skybox(const std::string &color_cubemap_key) {
        skybox.color_texture_id = resources.cubemaps.get(resources.cubemaps.find(color_cubemap_key)).texture_id;
    }

    ISystem *get_system(const std::string &name) { return systems.at(name).get(); }

    void remove_system(const std::string &name) { systems.erase(name); }

    void walk_gobject(GObject *root, uint32_t dirty_flags);
    void tick();
    // 获取屏幕上的一点对应的射线方向
    Vector3f get_screen_point_oritation(Vector2f screen_xy) const {
        float w_w = 400;
        float w_h = 300;
        Vector3f camera_up = camera.get_up_direction();
        Vector3f camera_forward = camera.get_orientation();
        Vector3f camera_right = camera_forward.cross(camera_up);
        float tan_fov = std::tan(camera.fov / 2);

        Vector3f ori = camera.get_orientation() +
                       camera_right * ((screen_xy.x / w_w - 0.5f) * tan_fov * w_w / w_h * 2.0f) +
                       camera_up * ((0.5f - screen_xy.y / w_h) * tan_fov * 2.0f);

        return ori.normalize();
    }

    std::shared_ptr<GObject> pick_up_object(Vector2f screen_xy) const { return nullptr; }

private:
    std::shared_ptr<GObject> root;
    std::unordered_map<std::string, std::unique_ptr<ISystem>> systems;
    SkyBox skybox;

    uint64_t tick_count = 0;
} world;

void Renderer::set_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    main_viewport = {x, y, width, height};
    world.camera.aspect = float(width) / float(height);
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
    glDepthFunc(GL_LESS);

    checkError();
}

//一般物体渲染
void LambertianPass::run() {
    // 初始化渲染配置
    glEnable(GL_CULL_FACE);  // 启用面剔除
    glEnable(GL_DEPTH_TEST); // 启用深度测试
    glDrawBuffer(GL_BACK);   // 渲染到后缓冲区
    Renderer::Viewport &v = renderer.main_viewport;
    glViewport(v.x, v.y, v.width, v.height);
    // 清除旧画面
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 绑定per_frame和per_object uniform buffer
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, per_frame_uniform.get_id());
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, per_object_uniform.get_id());

    // 填充per_frame uniform数据
    auto data = per_frame_uniform.map();
    // 透视投影矩阵
    data->view_perspective_matrix = world.camera.get_view_perspective_matrix().transpose();
    // 相机位置
    data->camera_position = world.camera.position;
    // 雾参数
    assert(world.fog_density >= 0.0f);
    data->fog_density = world.fog_density;
    data->fog_min_distance = world.fog_min_distance;
    // 灯光参数
    if (world.is_light_dirty) {
        data->ambient_light = world.ambient_light;
        if (world.pointlights.size() > POINTLIGNT_MAX) {
            std::cout << "超出最大点光源数量" << std::endl;
        }
        uint32_t count = std::min<uint32_t>(world.pointlights.size(), POINTLIGNT_MAX);
        for (uint32_t i = 0; i < count; ++i) {
            data->pointlight_list[i].position = world.pointlights[i].position;
            data->pointlight_list[i].intensity = world.pointlights[i].color * world.pointlights[i].factor;
        }
        data->pointlight_num = count;
        world.is_light_dirty = false;

        // std::cout << "Update light: count:" << count << std::endl;
    }
    // 填充结束
    per_frame_uniform.unmap();

    // 遍历所有part，绘制每一个part
    for (const GameObjectPart *p : parts) {
        // 填充per_object uniform buffer
        auto data = per_object_uniform.map();
        data->model_matrix = p->base_transform.transpose();      // 变换矩阵
        data->normal_matrix = p->base_normal_matrix.transpose(); // 法线变换矩阵
        per_object_uniform.unmap();

        // 查找并绑定材质
        const Material &material = resources.materials.get(p->material_id);
        material.bind();

        const Mesh &mesh = resources.meshes.get(p->mesh_id); // 网格数据

        glBindVertexArray(mesh.VAO_id);                                        // 绑定网格
        glDrawElements(p->topology, mesh.indices_count, GL_UNSIGNED_SHORT, 0); // 绘制
        checkError();
    }
}

// 渲染天空盒
void SkyBoxPass::run() {
    glEnable(GL_CULL_FACE);  // 启用面剔除
    glEnable(GL_DEPTH_TEST); // 启用深度测试
    glDrawBuffer(GL_BACK);   // 渲染到后缓冲区

    // 填充天空盒需要的参数（透视投影矩阵）
    auto data = skybox_uniform.map();
    data->skybox_view_perspective_matrix = world.camera.get_skybox_view_perspective_matrix().transpose();
    skybox_uniform.unmap();

    // 绑定天空盒专用着色器
    glUseProgram(shader_program_id);
    // 绑定uniform buffer
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, skybox_uniform.get_id());
    // 绑定天空盒纹理
    glActiveTexture(GL_TEXTURE0 + SKYBOX_TEXTURE_BINDIGN);
    assert(skybox_texture_id != 0); // 天空纹理必须存在
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_id);
    // 获取天空盒的网格（向内的Cube）
    const Mesh &mesh = resources.meshes.get(mesh_id);

    glBindVertexArray(mesh.VAO_id);                                         // 绑定网格
    glDrawElements(GL_TRIANGLES, mesh.indices_count, GL_UNSIGNED_SHORT, 0); // 绘制
    checkError();
}

void PickupPass::run() {
    set_framebuffer_size(renderer.main_viewport.width, renderer.main_viewport.height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_pickup);
    glViewport(0, 0, renderer.main_viewport.width, renderer.main_viewport.height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //todo

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
} 

namespace Assets {

const std::vector<Vector3f> skybox_cube_vertices = {{-1.0, -1.0, -1.0}, {1.0, -1.0, -1.0}, {1.0, 1.0, -1.0},
                                                    {-1.0, 1.0, -1.0},  {-1.0, -1.0, 1.0}, {1.0, -1.0, 1.0},
                                                    {1.0, 1.0, 1.0},    {-1.0, 1.0, 1.0}};

const std::vector<uint16_t> skybox_cube_indices = {3, 0, 1, 1, 2, 3, 6, 7, 3, 3, 2, 6, 0, 3, 7, 7, 4, 0,
                                                   5, 6, 2, 2, 1, 5, 6, 5, 4, 4, 7, 6, 0, 4, 5, 5, 1, 0};

const std::vector<Vertex> plane_vertices = {
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

const std::vector<uint16_t> plane_indices = {3, 2, 0, 2, 1, 0};
} // namespace Assets

void init_resource() {
    // 从json加载大部分的资源
    resources.load_json("assets/resources.json");
    // 材质资源比较特殊，暂时通过硬编码加载
    {
        // 平滑着色材质
        Vector3f color_while{1.0f, 1.0f, 1.0f};
        resources.add_material("wood_flat",
                               MaterialDesc{"flat", {{2, sizeof(Vector3f), &color_while}}, {{3, "wood_diffusion"}}});
    }

    {
        // 单一颜色材质
        MaterialDesc green_material_desc;
        Vector3f color_green{0.0f, 1.0f, 0.0f};
        green_material_desc.shader_name = "single_color";
        green_material_desc.uniforms.emplace_back(
            MaterialDesc::UniformDataDesc{2, sizeof(Vector3f), color_green.data()});
        resources.add_material("default", green_material_desc);
    }
    // 部分硬编码的mesh
    resources.add_mesh("default", {}, {});
    resources.add_mesh("plane", Assets::plane_vertices, Assets::plane_indices);

    // 添加天空盒的mesh，因为格式不一样所以单独处理
    {
        unsigned int vao_id, ibo_id, vbo_id;
        glGenVertexArrays(1, &vao_id);
        glGenBuffers(1, &ibo_id);
        glGenBuffers(1, &vbo_id);

        glBindVertexArray(vao_id);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, Assets::skybox_cube_vertices.size() * sizeof(Vector3f),
                     Assets::skybox_cube_vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, Assets::skybox_cube_indices.size() * sizeof(uint16_t),
                     Assets::skybox_cube_indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3f), (void *)0);
        glEnableVertexAttribArray(0);

        checkError();

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        resources.meshes.add("skybox_cube", Mesh{vao_id, (uint32_t)Assets::skybox_cube_indices.size()});
        // 注册销毁用回调函数在程序结束时调用
        resources.deconstructors.emplace_back([vao_id, vbo_id, ibo_id]() {
            glDeleteVertexArrays(1, &vao_id);
            glDeleteBuffers(1, &vbo_id);
            glDeleteBuffers(1, &ibo_id);
            checkError();
        });
    }
}

// 从json加载一个Vector3f
Vector3f load_vec3(const SimpleJson::JsonObject &json) {
    assert(json.get_type() == SimpleJson::JsonType::List);
    const std::vector<SimpleJson::JsonObject> &numbers = json.get_list();
    return {(float)numbers[0].get_number(), (float)numbers[1].get_number(), (float)numbers[2].get_number()};
}
// 从json加载transform，对不完整或不存在的取默认值
Transform load_transform(const SimpleJson::JsonObject &json) {
    Transform trans{
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
    };
    if (json.has("transform")) {
        const SimpleJson::JsonObject &t = json["transform"];
        if (t.has("position")) {
            trans.position = load_vec3(t["position"]);
        }
        if (t.has("rotation")) {
            trans.rotation = load_vec3(t["rotation"]);
        }
        if (t.has("scale")) {
            trans.scale = load_vec3(t["scale"]);
        }
    }

    return trans;
}

// 从json递归加载节点
void load_node_from_json(const SimpleJson::JsonObject &node, GObject *root) {
    for (const SimpleJson::JsonObject &object_desc : node.get_list()) {
        GObjectDesc desc{load_transform(object_desc), {}};
        for (const SimpleJson::JsonObject &part_desc : object_desc["parts"].get_list()) {
            desc.parts.emplace_back(part_desc["mesh"].get_string(), part_desc["material"].get_string());
        }
        const std::string &name = object_desc.has("name") ? object_desc["name"].get_string() : "";
        root->attach_child(std::make_shared<GObject>(std::move(desc), name));
        if (object_desc.has("children")) {
            load_node_from_json(object_desc["children"], root->get_children().back().get());
        }
    }
}

// 从json文件加载场景
void load_scene_from_json(const std::string &path) {
    SimpleJson::JsonObject json = SimpleJson::parse_file(path);
    // 加载天空盒
    if (!json.has("skybox")) {
        std::cerr << "Skybox is required for a scene" << std::endl;
        exit(-1);
    }

    world.set_skybox(json["skybox"]["specular_texture"].get_string());
    // 加载物体
    if (json.has("root")) {
        load_node_from_json(json["root"], world.get_root().get());
    }
    // 加载点光源
    if (json.has("pointlights")) {
        for (const SimpleJson::JsonObject &pointlight_desc : json["pointlights"].get_list()) {
            PointLight &light = world.pointlights.emplace_back();
            light.position = load_vec3(pointlight_desc["position"]);
            light.color = load_vec3(pointlight_desc["color"]);
            light.factor = (float)pointlight_desc["factor"].get_number();
        }
    }
}
void init_start_scene() {
    load_scene_from_json("assets/scene1.json"); // 整个场景的所有物体都从json加载了
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

// 递归更新物体
void World::walk_gobject(GObject *root, uint32_t dirty_flags) {
    dirty_flags |= root->is_relat_dirty;
    if (dirty_flags) {
        if (root->has_parent()) {
            // 子节点的transform为父节点的transform叠加上自身的transform
            root->relate_model_matrix =
                root->get_parent().lock()->relate_model_matrix * root->transform.transform_matrix();
            root->relate_normal_matrix =
                root->get_parent().lock()->relate_normal_matrix * root->transform.normal_matrix();
        } else {
            root->relate_model_matrix = root->transform.transform_matrix();
            root->relate_normal_matrix = root->transform.normal_matrix();
        }

        for (GameObjectPart &p : root->get_parts()) {
            // 更新自身每一个part的transform
            p.base_transform = root->relate_model_matrix * p.model_matrix;
            p.base_normal_matrix = root->relate_normal_matrix * p.normal_matrix;
        }
        root->is_relat_dirty = false;
    }
    //将此物体提交渲染
    for (GameObjectPart &p : root->get_parts()) {
        // 提交自身每一个part
        renderer.accept(&p);
    }

    for (auto &child : root->get_children()) {
        walk_gobject(child.get(), dirty_flags); // 更新子节点
    }
}

void World::tick() {
    tick_count++;

    clock.update(); // 更新时钟
    // 调用所有的系统
    for (const auto &[name, sys] : systems) {
        if (sys->enable) {
            sys->tick();
        }
    }
    // 递归更新所有物体
    walk_gobject(this->root.get(), 0);

    //上传天空盒
    renderer.skybox_pass->set_skybox(skybox.color_texture_id);
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
            // world.pointlights[0].enabled = false;
            // world.pointlights[1].enabled = false;
            // world.is_light_dirty = true;
        }

        if (!is_right_button_down && input.is_right_button_down()) {
            // world.pointlights[0].enabled = true;
            // world.pointlights[1].enabled = true;
            // world.is_light_dirty = true;
        }

        if (input.is_middle_button_down()) {
            auto [dx, dy] = input.get_mouse_move().v;
            // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
            const float rotate_speed = 0.003f;
            Camera &desc = world.camera;
            desc.rotation.z += dx * rotate_speed;
            desc.rotation.y -= dy * rotate_speed;
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

        if (IS_KEYDOWN(VK_UP)) {
            world.fog_density += 0.00001f * delta;
        }

        if (IS_KEYDOWN(VK_DOWN)) {
            world.fog_density -= 0.00001f * delta;
            world.fog_density = std::max(world.fog_density, 0.0f);
        }
    }

    void on_attach() override { ball = world.get_root()->get_child_by_name("球"); }

    void tick() override {
        handle_keyboard(world.clock.get_delta());
        handle_mouse();

        ball->transform.rotation.x += world.clock.get_delta() * 0.001f;
        ball->transform.rotation.y += world.clock.get_delta() * 0.003f;
        ball->is_relat_dirty = true;
        world.pointlights[0].position.x = 20.0f * sinf(world.get_tick_count() * 0.01f);
        world.is_light_dirty = true;
    }

private:
    std::shared_ptr<GObject> ball;
};

int main(int argc, char **argv) {
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
    glutReshapeFunc([](int w, int h) { renderer.set_viewport(0, 0, w, h); });
    glutMouseFunc(handle_mouse_click);
    glutMotionFunc(handle_mouse_move);
    glutPassiveMotionFunc(handle_mouse_move);

    // 初始化opengl
    setup_opengl();

    // 初始化资源和加载资源
    init_resource();

    //初始化渲染相关pass等
    renderer.init();
    renderer.set_viewport(0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT); // 初始化时也需要设置一下视口

    // 加载初始场景
    init_start_scene();

    // 注册系统
    world.register_system(new MoveSystem("move"));

    // 注册在退出时执行的清理操作
    atexit([] {
        world.clear();
        resources.clear();
        renderer.clear();
        std::cout << "Exit!\n";
    });

    world.clock.update(); // 重置一下时钟

    // XX，启动！
    glutMainLoop();

    return 0;
}