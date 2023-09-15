#pragma once
#include "CGmath.h"
#include "render_aspect.h"
#include <GL/glew.h>
#include <GL/glext.h>
#include <assert.h>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <unordered_map>


inline void checkError() {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "GL error 0x%X: %s\n", error, gluErrorString(error));
    }
}

inline unsigned int calculate_fps(float delta_time) {
    const float ratio = 0.1f;
    static float avarage_frame_time = std::nan("");

    if (std::isnormal(avarage_frame_time)) {
        avarage_frame_time =
            avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time);
}

struct CameraDesc {
    Vector3f position;
    Vector3f rotation;
    uint32_t window_width, window_height;
    float fov, far_z, near_z;
};

struct GameObjectPartDesc {
    std::string resource_key_mesh;
    std::string resource_key_material;
    uint32_t topology;
    Matrix model_matrix;

    GameObjectPartDesc(const std::string &resource_key_mesh = "default",
                       const std::string &resource_key_material = "default",
                       uint32_t topology = GL_TRIANGLES, const Matrix &model_matrix = Matrix::identity())
        : resource_key_mesh(resource_key_mesh),
          resource_key_material(resource_key_material), topology(topology),
          model_matrix(model_matrix) {}
};

struct GameObjectRenderDesc {
    uint32_t id;
    std::vector<GameObjectPartDesc> parts;
};

struct GameObjectPartId {
    uint32_t gameobject_id;
    uint32_t part_index;

    bool operator==(const GameObjectPartId &id) const {
        return gameobject_id == id.gameobject_id && part_index == id.part_index;
    }
};

template <> class std::hash<GameObjectPartId> {
public:
    size_t operator()(const GameObjectPartId &id) const {
        return (size_t)id.gameobject_id << 4 ^ (size_t)id.part_index;
    }
};

struct LightDesc {
    bool is_point;
    Vector3f position_or_direction;
    Vector3f color;
    float identity;
};

struct ResouceItem{
    virtual ~ResouceItem() = default;
};
class RenderReousce {
public:
    uint32_t find_material(const std::string &key) {
        if (auto it = material_look_up.find(key);
            it != material_look_up.end()) {
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

    const PhongMaterial &get_material(uint32_t id) {
        return material_container[id];
    }
    
    //保存在退出时释放
    template<class T>
    void add_raw_resource(T&& item){
        pool.emplace_back(std::make_unique<T>(std::move(item)));
    }

private:
    std::unordered_map<std::string, uint32_t> mesh_look_up;
    std::vector<Mesh> mesh_container;

    std::unordered_map<std::string, uint32_t> material_look_up;
    std::vector<PhongMaterial> material_container;

    std::vector<std::unique_ptr<ResouceItem>> pool;//保存在这里以析构释放资源
};

struct RenderSwapData {
    std::optional<CameraDesc> camera_desc;
    std::vector<GameObjectRenderDesc> gameobject_desc;
    std::vector<uint32_t> game_object_to_delete;
    std::vector<LightDesc> light_desc;

    void clear() {
        camera_desc.reset();
        gameobject_desc.clear();
        game_object_to_delete.clear();
        light_desc.clear();
    }
};

struct LockedSwapData {
    RenderSwapData &data;
    std::lock_guard<std::mutex> lock;

    LockedSwapData(RenderSwapData &data, std::mutex &mutx)
        : data(data), lock(mutx, std::adopt_lock) {}

    RenderSwapData *operator->() { return &data; }

    void add_dirty_gameobject(GameObjectRenderDesc &&desc) {
        data.gameobject_desc.emplace_back(std::move(desc));
    }

    void add_light(const LightDesc &desc) {
        data.light_desc.emplace_back(desc);
    }

    void set_camera(const CameraDesc &desc) {
        data.camera_desc.emplace(desc); 
    }

    void delete_game_object(const uint32_t id) {
        data.game_object_to_delete.push_back(id);
    }
};

struct RenderScene {
    Camera main_camera;
    std::unordered_map<GameObjectPartId, RenderItem> item_list;
    uint32_t window_width;
    uint32_t window_height;

    void update(RenderReousce &resources, const RenderSwapData &swap_data) {

        if (swap_data.camera_desc.has_value()) {
            const CameraDesc &desc = swap_data.camera_desc.value();
            main_camera.position = desc.position;
            main_camera.rotation = desc.rotation;
            window_width = desc.window_width;
            window_height = desc.window_height;
            main_camera.set_view_perspective_matrix(float(desc.window_width) / float(desc.window_height), desc.fov,
                                                    desc.near_z, desc.far_z);
        }

        // 先更新Object再删除，防止删除的Object因为更新而重新加入
        for (const GameObjectRenderDesc &object : swap_data.gameobject_desc) {
            for (uint32_t i = 0; i < object.parts.size(); i++) {
                GameObjectPartId part_id{object.id, i};
                const GameObjectPartDesc &part = object.parts[i];

                RenderItem &item = item_list[part_id];
                item.mesh_id = resources.find_mesh(part.resource_key_mesh);
                item.material_id =
                    resources.find_material(part.resource_key_material);
                item.topology = part.topology;
                item.model_matrix = part.model_matrix;
            }
        }

        for (uint32_t id : swap_data.game_object_to_delete) {
            // 删除此object的所有part
            for (uint32_t i = 0; true; i++) {
                GameObjectPartId part_id{id, i};
                if (auto it = item_list.find(part_id); it != item_list.end()) {
                    item_list.erase(it);
                } else {
                    break;
                }
            }
        }

        for (const LightDesc &light : swap_data.light_desc) {
        }
    }
};

extern void render_thread_func();
class Renderer {
public:
    Renderer() {}

    LockedSwapData get_logic_swap_data() {
        // 构造LockedSwapData会自动加锁，析构自动解锁
        swap_data_lock.lock();
        return LockedSwapData(swap_data[0], swap_data_lock);
    }

    void render() {
        scene.update(resouces, swap_data[1]);

        glDrawBuffer(GL_BACK);//渲染到后缓冲区
        glViewport(0, 0, scene.window_width, scene.window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(
            scene.main_camera.view_perspective_matrix.transpose().data());

        glMatrixMode(GL_MODELVIEW);
        for (const auto &[id, item] : scene.item_list) {
            const Mesh &mesh = resouces.get_mesh(item.mesh_id);
            const PhongMaterial &material =
                resouces.get_material(item.material_id);

            // glMaterialfv(GL_FRONT, GL_DIFFUSE, material.diffusion.data());

            glLoadMatrixf(item.model_matrix.transpose().data());
            glBegin(item.topology);
            for (uint32_t i = 0;i < mesh.indices_count;i++) {
                const Vertex &v = mesh.vertices[mesh.indices[i]];
                glColor3fv(v.color.data());
                glVertex3fv(v.position.data());
            }
            glEnd();
        }

        glFlush();
        checkError();
        
        do_swap();
    }

    void do_swap() {
        // 由渲染线程调用
        swap_data[1].clear();
        {
            std::lock_guard<std::mutex> lock(swap_data_lock);
            std::swap(swap_data[0], swap_data[1]);
        }
    }

    bool render_thread_should_exit{false};

    ~Renderer() {
        if (!render_thread_should_exit){
            std::cerr << "渲染线程意外退出！！！" << std::endl;
        }
        render_thread.join();
    }

private:
    friend class GlobalRuntime;
    friend void init_render_resource();
    std::thread render_thread;
    /*
    由渲染线程和逻辑线程访问，一个只能访问对应的那一个
    再渲染线程完成一次渲染后，会进行交换以访问下一帧的数据，但是此时
    1.
    逻辑帧还没有完成，此时交换到一个空的SwapData，照常执行，使用旧数据继续渲染
    2. 逻辑帧完成，此时交换到存在数据的SwapData，将数据同步到RenderScene，渲染
    3. 在交换时，需要对swap_data加锁，但读取时不需要
    对于逻辑线程，再完成逻辑的同时填充swap_data，在每次需要向其中添加数据时，加锁
    逻辑线程不管交换和释放，但写数据时的加锁时间尽量短，因为可能阻塞渲染线程
    */
    std::mutex swap_data_lock;
    RenderSwapData swap_data[2];

    RenderScene scene;
    RenderReousce resouces;

    void start_thread() { render_thread = std::thread(render_thread_func); }

    void terminal_thread() {
        assert(!render_thread_should_exit);
        render_thread_should_exit = true;
        // render_thread.join();
    }
};