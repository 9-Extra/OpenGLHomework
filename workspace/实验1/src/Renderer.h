#pragma once

#include <memory>

#include "Passes.h"
#include "RenderItem.h"
#include "utils.h"

// 渲染管理器，包含所有渲染需要的数据供pass使用, 在world tick时各种组件会将渲染数据写到这里
class Renderer final {
public:
    struct Viewport {
        GLint x;
        GLint y;
        GLsizei width;
        GLsizei height;
    } main_viewport; // 主视口

    Camera main_camera;             // 主像机
    bool is_camera_updated = false; // 是否是主视口的主相机
    void *active_camera;            // 实际上是CpntCamera的owner的指针

    Vector3f ambient_light = {0.02f, 0.02f, 0.02f}; // 环境光
    std::vector<PointLight> pointlights;            // 点光源

    float fog_min_distance = 5.0f; // 雾开始的距离
    float fog_density = 0.001f;    // 雾强度

    void init() {
        lambertian_pass = std::make_unique<LambertianPass>();
        single_color_pass = std::make_unique<SingleColorPass>();
        pickup_pass = std::make_unique<PickupPass>();
    }

    void accept(GameObjectPart *part) { single_color_pass->accept(part); }

    void render();

    void set_viewport(GLint x, GLint y, GLsizei width, GLsizei height) { main_viewport = {x, y, width, height}; }

    void clear() {
        lambertian_pass.reset();
        single_color_pass.reset();
        pickup_pass.reset();
    }

private:
    // passes
    std::unique_ptr<LambertianPass> lambertian_pass;
    std::unique_ptr<SingleColorPass> single_color_pass;
    std::unique_ptr<PickupPass> pickup_pass;
};

extern Renderer renderer;