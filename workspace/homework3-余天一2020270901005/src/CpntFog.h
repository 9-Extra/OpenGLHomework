#pragma once

#include "Renderer.h"
#include "GObject.h"

// 光源组件
class CpntFog: public Component{
public:
    CpntFog(float fog_min_distance = 5.0f, float fog_density = 0.001f): fog_min_distance(fog_min_distance), fog_density(fog_density) {}

    float fog_min_distance; // 雾开始的距离
    float fog_density;    // 雾强度

    virtual void tick() override {
        assert(get_owner() != nullptr);
        
    }
};

