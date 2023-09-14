#pragma once
#include "CGmath.h"
#include <assert.h>
#include <cstdio>
#include <GL/glew.h>
#include <GL/glext.h>
#include <vector>
#include "render.h"
#include "GuidAlloctor.h"

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
    World(Renderer& renderer): renderer(renderer) {}

    uint32_t create_object(){
        uint32_t id = game_object_id_alloctor.alloc_id();
        objects.emplace(id, GObject(id));
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

    const CameraDesc& get_camera() const{
        return camera;
    }

    CameraDesc& set_camera(){
        is_camera_dirty = true;
        return camera;
    } 
private:
    GuidAlloctor game_object_id_alloctor;
    CameraDesc camera;
    bool is_camera_dirty{false};
    Renderer& renderer;
};