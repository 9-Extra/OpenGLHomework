#pragma once
#include "../utils/CGmath.h"
#include "engine/render/Render.h"
#include <vector>
#include <memory>
struct GObjectDesc {
    Vector3f position{0.0, 0.0, 0.0};
    Vector3f rotation{0.0, 0.0, 0.0}; // zxy顺序
    Vector3f scale{1.0, 1.0, 1.0};
    std::vector<GameObjectPartDesc> parts;
};

//物体的基类，可以直接使用
//继承enable_shared_from_this必须使用public
class GObject : public std::enable_shared_from_this<GObject>{
public:
    Vector3f position{0.0, 0.0, 0.0};
    Vector3f rotation{0.0, 0.0, 0.0}; // zxy顺序
    Vector3f scale{1.0, 1.0, 1.0};
    
    GObject() {};
    GObject(GObjectDesc&& desc) {
        this->position = desc.position;
        this->rotation = desc.rotation;
        this->scale = desc.scale;
        this->parts = std::move(desc.parts);
    };

    GObject(const GObjectDesc& desc) {
        this->from_desc(desc);
    };

    std::vector<GameObjectPartDesc>& set_parts(){
        mark_dirty();
        return parts;
    }

    //可以重载
    virtual void tick() {};

    GObjectDesc to_desc(){
        return {
            position,
            rotation,
            scale,
            parts
        };
    }

    void mark_dirty(){
        is_render_dirty = true;
    }

    void from_desc(const GObjectDesc& desc){
        position = desc.position;
        rotation = desc.rotation;
        scale = desc.scale;
        parts = desc.parts;
        mark_dirty();
    }
private:
    friend class World;

    std::vector<GameObjectPartDesc> parts;
    bool is_render_dirty{true};

    void update_render(LockedSwapData &swap_data);
};
