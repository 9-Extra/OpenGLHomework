#pragma once
#include "../utils/CGmath.h"
#include <assert.h>
#include <cstdio>
#include <GL/glew.h>
#include <GL/glext.h>
#include <vector>
#include "../render/Render.h"
#include "../utils/GuidAlloctor.h"
#include "../command/Command.h"

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

    void update_render(LockedSwapData &swap_data);
};

class World {
public:
    uint32_t window_width,window_height; 
    std::unordered_map<uint32_t, GObject> objects;
    World(Renderer& renderer): renderer(renderer) {
        init_start_scene();
        update_swap_data();
    }

    uint32_t create_object();

    uint32_t create_object(const GObjectDesc &desc);

    uint32_t create_object(GObjectDesc &&desc);

    void kill_object(uint32_t id){
        if (objects.erase(id) == 0){
            std::cerr << "试图删除不存在的object!" << std::endl;
        }
        renderer.get_logic_swap_data().delete_game_object(id);
    }

    void tick();

    void update_swap_data();

    // 获取目视方向
    Vector3f get_camera_oritation() const;

    //获取相机正上方
    Vector3f get_camera_up() const;

    //获取屏幕上的一点对应的射线方向
    Vector3f get_screen_point_oritation(Vector2f screen_xy) const;

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

    void init_start_scene();
};