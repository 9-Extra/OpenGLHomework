#pragma once
#include "CGmath.h"
#include <assert.h>
#include <cstdio>
#include <GL/glew.h>
#include <GL/glext.h>
#include <vector>
#include "render.h"
#include "GuidAlloctor.h"
#include "Command.h"

struct GObjectDesc {
    Vector3f position{0.0, 0.0, 0.0};
    Vector3f rotation{0.0, 0.0, 0.0}; // zxy顺序
    Vector3f scale{1.0, 1.0, 1.0};
    std::vector<GameObjectPartDesc> parts;
};
class GObject final{
public:
    void set_position(const Vector3f& position){
        this->position = position;
        is_render_dirty = true;
    }

    void set_rotation(const Vector3f& rotation){
        this->rotation = rotation;
        is_render_dirty = true;
    }

    void set_scale(const Vector3f& scale){
        this->scale = scale;
        is_render_dirty = true;
    }

    std::vector<GameObjectPartDesc>& set_parts(){
        is_render_dirty = true;
        return parts;
    }

    GObjectDesc to_desc(){
        return {
            position,
            rotation,
            scale,
            parts
        };
    }

    void from_desc(const GObjectDesc& desc){
        position = desc.position;
        rotation = desc.rotation;
        scale = desc.scale;
        parts = desc.parts;
        is_render_dirty = true;
    }
private:
    friend class World;
    const uint32_t id;
    
    Vector3f position{0.0, 0.0, 0.0};
    Vector3f rotation{0.0, 0.0, 0.0}; // zxy顺序
    Vector3f scale{1.0, 1.0, 1.0};

    std::vector<GameObjectPartDesc> parts;
    bool is_render_dirty{true};
    
    GObject(const uint32_t id): id(id) {}

    void update_render(LockedSwapData& swap_data){
        if (is_render_dirty){
            Matrix transform = 
                Matrix::translate(position) *
                Matrix::rotate(rotation) *
                Matrix::scale(scale);

            std::vector<GameObjectPartDesc> dirty_parts;
            for(const GameObjectPartDesc& p: parts){
                dirty_parts.emplace_back(
                    p.resource_key_mesh,
                    p.resource_key_material,
                    p.topology,
                    p.model_matrix * transform
                );
            }
            swap_data.add_dirty_gameobject(GameObjectRenderDesc{id, std::move(dirty_parts)});
            is_render_dirty = false;
        }
    }
};

class World {
public:
    std::unordered_map<uint32_t, GObject> objects;
    World(Renderer& renderer): renderer(renderer) {
        init_start_scene();
    }

    uint32_t create_object(){
        uint32_t id = game_object_id_alloctor.alloc_id();
        objects.emplace(id, GObject(id));
        return id;
    }

    uint32_t create_object(const GObjectDesc& desc){
        uint32_t id = create_object();
        GObject& obj = objects.at(id);
        obj.position = desc.position;
        obj.rotation = desc.rotation;
        obj.scale = desc.scale;
        obj.parts = desc.parts;
        return id;
    }

    void kill_object(uint32_t id){
        if (objects.erase(id) == 0){
            std::cerr << "试图删除不存在的object!" << std::endl;
        }
        renderer.get_logic_swap_data().delete_game_object(id);
    }

    void tick(float delta){
        for(auto& [id, obj]: objects){
            //obj.tick(delta);
        }

        LockedSwapData swap_data = renderer.get_logic_swap_data();
        if (is_camera_dirty){
            swap_data.set_camera(camera);
            is_camera_dirty = false;
        }
        for(auto& [id, obj]: objects){
            obj.update_render(swap_data);
        }
    }

    // 获取目视方向
    Vector3f get_camera_oritation() const{
        float pitch = camera.rotation[1];
        float yaw = camera.rotation[2];
        return {sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(pitch) * cosf(yaw)};
    }

    //获取相机正上方
    Vector3f get_camera_up() const{
        float sp = sinf(camera.rotation[1]);
        float cp = cosf(camera.rotation[1]);
        float cr = cosf(camera.rotation[0]);
        float sr = sinf(camera.rotation[0]);
        float sy = sinf(camera.rotation[2]);
        float cy = cosf(camera.rotation[2]);

        return {-sp * sy * cr - sr * cy, cp * cr, sp * cr * cy - sr * sy};
    }

    //获取屏幕上的一点对应的射线方向
    Vector3f get_screen_point_oritation(Vector2f screen_xy) const{
        float w_w = float(camera.window_width);
        float w_h = float(camera.window_height);
        Vector3f camera_up = get_camera_up();
        Vector3f camera_forward = get_camera_oritation();
        Vector3f camera_right = camera_forward.cross(camera_up);
        float tan_fov = std::tan(camera.fov / 2); 

        Vector3f ori = get_camera_oritation() + 
            camera_right * ((screen_xy.x / w_w - 0.5f) * tan_fov * w_w / w_h * 2.0f) +
            camera_up * ((0.5f - screen_xy.y / w_h) * tan_fov * 2.0f);

        return ori.normalize();
    }

    const CameraDesc& get_camera() const{
        return camera;
    }

    CameraDesc& set_camera(){
        is_camera_dirty= true;
        return camera;
    } 
private:
    GuidAlloctor game_object_id_alloctor;
    CameraDesc camera;
    bool is_camera_dirty{false};
    Renderer& renderer;

    void init_start_scene() {
    {
        CameraDesc &desc = this->set_camera();
        desc.position = {0.0f, 0.0f, 0.0f};
        desc.rotation = {0.0f, 0.0f, 0.0f};
        desc.fov = 1.57f;
        desc.near_z = 1.0f;
        desc.far_z = 1000.0f;
    }
    {
        create_object(GObjectDesc{
            {0.0f, 0.0f, -10.0f},
            {0.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 1.0f},
            {GameObjectPartDesc{"cube", "default"}}
        });
    }

    {
        GObject &platform = this->objects.at(this->create_object());
        platform.set_parts().emplace_back(
            GameObjectPartDesc{"platform", "default"});
        platform.set_position({0.0, -2.0, -10.0});
        platform.set_scale({4.0, 0.1, 4.0});
    }
}
};