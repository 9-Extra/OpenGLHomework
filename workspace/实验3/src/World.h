#pragma once

#include <unordered_map>

#include "GObject.h"
#include "clock.h"

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
    Clock clock;

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
        root = std::make_shared<GObject>(Transform{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, "root");
    }

    void register_system(ISystem *system) {
        assert(system != nullptr && systems.find(system->get_name()) == systems.end());
        systems.emplace(system->get_name(), system);
        system->on_attach();
    }

    ISystem *get_system(const std::string &name) { return systems.at(name).get(); }

    void remove_system(const std::string &name) { systems.erase(name); }

    void tick();
    // 获取屏幕上的一点对应的射线方向
    // Vector3f get_screen_point_oritation(Vector2f screen_xy) const {
    //     float w_w = 400;
    //     float w_h = 300;
    //     Vector3f camera_up = camera.get_up_direction();
    //     Vector3f camera_forward = camera.get_orientation();
    //     Vector3f camera_right = camera_forward.cross(camera_up);
    //     float tan_fov = std::tan(camera.fov / 2);

    //     Vector3f ori = camera.get_orientation() +
    //                    camera_right * ((screen_xy.x / w_w - 0.5f) * tan_fov * w_w / w_h * 2.0f) +
    //                    camera_up * ((0.5f - screen_xy.y / w_h) * tan_fov * 2.0f);

    //     return ori.normalize();
    // }

    std::shared_ptr<GObject> pick_up_object(Vector2f screen_xy) const { return nullptr; }

private:
    std::shared_ptr<GObject> root;
    std::unordered_map<std::string, std::unique_ptr<ISystem>> systems;

    uint64_t tick_count = 0;
};

extern World world;

void load_scene_from_json(const std::string &path);