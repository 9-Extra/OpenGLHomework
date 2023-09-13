#pragma once
#include "YGraphics.h"
#include <thread>
#include <mutex>
#include <tuple>
#include <optional>
#include <unordered_map>
#include <iostream>
#include "render_aspect.h"

struct CameraDesc{
    Vector3f position;
    Vector3f oritation;
    float ratio;
    float fov, far_z, near_z;
};

struct GameObjectPartDesc{
    std::string resource_key_mesh;
    std::string resource_key_material;
    Matrix model_matrix;
};

struct GameObjectDesc{
    uint32_t id;
    std::vector<GameObjectPartDesc> parts;
};

struct GameObjectPartId{
    uint32_t gameobject_id;
    uint32_t part_index;  

    bool operator==(const GameObjectPartId& id) const{
        return gameobject_id == id.gameobject_id && part_index == id.part_index;
    }
};

template <>
class std::hash<GameObjectPartId>{
public:
    size_t operator()(const GameObjectPartId& id) const{
        return (size_t)id.gameobject_id << 4 ^ (size_t)id.part_index;
    }
};

struct LightDesc{
    bool is_point;
    Vector3f position_or_direction;
    Vector3f color;
    float identity;
};

class RenderReousce{
public:
    uint32_t find_material(const std::string& key){
        if (auto it = material_look_up.find(key);it != material_look_up.end()){
            return it->second;
        } else {
            std::cerr << "找不到material: " << key;
            exit(-1);
        }
    }

    void add_material(const std::string& key, PhongMaterial&& material){
        uint32_t id = material_container.size();
        material_container.emplace_back(material);
        material_look_up[key] = id;
    }

    uint32_t find_mesh(const std::string& key){
        if (auto it = mesh_look_up.find(key);it != mesh_look_up.end()){
            return it->second;
        } else {
            std::cerr << "找不到mesh: " << key;
            exit(-1);
        }
    }

    void add_mesh(const std::string& key, Mesh&& mesh){
        uint32_t id = mesh_container.size();
        mesh_container.emplace_back(mesh);
        mesh_look_up[key] = id;
    }
private:
    std::unordered_map<std::string, uint32_t> mesh_look_up;
    std::vector<Mesh> mesh_container;

    std::unordered_map<std::string, uint32_t> material_look_up;
    std::vector<PhongMaterial> material_container;
};

struct RenderSwapData{
    std::optional<CameraDesc> camera_desc;
    std::vector<GameObjectDesc> gameobject_desc;
    std::vector<LightDesc> light_desc;

    void clear(){
        camera_desc.reset();
        gameobject_desc.clear();
    }
};

struct LockedSwapData{
    RenderSwapData& data;
    std::lock_guard<std::mutex> lock;

    LockedSwapData(RenderSwapData& data, std::mutex& mutx): data(data), lock(mutx){}

    RenderSwapData* operator->(){
        return &data;
    }
};

struct RenderScene{
    Camera main_camera;
    std::unordered_map<GameObjectPartId, RenderItem> item_list;

    void update(RenderReousce& resources, const RenderSwapData& swap_data){
        if (swap_data.camera_desc.has_value()){
            main_camera.position = swap_data.camera_desc->position;
            main_camera.rotation = swap_data.camera_desc->oritation;
        }

        for(const GameObjectDesc& object: swap_data.gameobject_desc){
            for(uint32_t i = 0;i < object.parts.size();i++){
                GameObjectPartId part_id{object.id, i};
                const GameObjectPartDesc& part = object.parts[i];

                RenderItem& item = item_list[part_id];
                item.mesh_id = resources.find_mesh(part.resource_key_mesh);
                item.material_id = resources.find_material(part.resource_key_material);
                item.model_matrix = part.model_matrix;
            }
        }

        for(const LightDesc& light : swap_data.light_desc){

        }
    }
};

extern void render_thread_func();
class Renderer{
public:
    Renderer() {

    }

    LockedSwapData get_logic_swap_data(){
        //构造LockedSwapData会自动加锁，析构自动解锁
        return LockedSwapData(swap_data[0], swap_data_lock);
    }

    const RenderSwapData& get_render_swap_data(){
        return swap_data[1];
    }

    void do_swap(){
        //由渲染线程调用
        swap_data[1].clear();
        {
            std::lock_guard<std::mutex> lock(swap_data_lock);
            std::swap(swap_data[0], swap_data[1]);
        }
    }

    bool render_thread_should_exit{false};

    ~Renderer(){
        assert(render_thread_should_exit);
    }

private:
    friend class GlobalRuntime;
    std::thread render_thread;
    /*
    由渲染线程和逻辑线程访问，一个只能访问对应的那一个
    再渲染线程完成一次渲染后，会进行交换以访问下一帧的数据，但是此时
    1. 逻辑帧还没有完成，此时交换到一个空的SwapData，照常执行，使用旧数据继续渲染
    2. 逻辑帧完成，此时交换到存在数据的SwapData，将数据同步到RenderScene，渲染
    3. 在交换时，需要对swap_data加锁，但读取时不需要
    对于逻辑线程，再完成逻辑的同时填充swap_data，在每次需要向其中添加数据时，加锁
    逻辑线程不管交换和释放，但写数据时的加锁时间尽量短，因为可能阻塞渲染线程
    */
    std::mutex swap_data_lock;
    RenderSwapData swap_data[2];

    RenderScene scene;

    void start_thread(){
        render_thread = std::thread(render_thread_func);
    }

    void terminal_thread(){
        render_thread_should_exit = true;
        render_thread.join();
    }

};

inline void draw_item(){




}