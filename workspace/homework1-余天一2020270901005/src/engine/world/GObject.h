#pragma once
#include "../utils/CGmath.h"
#include "engine/render/Render.h"
#include <vector>

struct GObjectDesc {
    Vector3f position{0.0, 0.0, 0.0};
    Vector3f rotation{0.0, 0.0, 0.0}; // zxy顺序
    Vector3f scale{1.0, 1.0, 1.0};
    std::vector<GameObjectPartDesc> parts;
};
class GObject{
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

    //可以重载
    void tick() {};

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
