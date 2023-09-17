#include "World.h"

void World::tick() {
    for (auto &[id, obj] : objects) {
        obj->tick();
    }

    update_swap_data();
}
void World::update_swap_data() {
    LockedSwapData swap_data = renderer.get_logic_swap_data();
    if (is_camera_dirty) {
        swap_data.set_camera(camera);
        is_camera_dirty = false;
    }
    for (auto &[id, obj] : objects) {
        obj->update_render(swap_data);
    }
}
Vector3f World::get_camera_oritation() const {
    float pitch = camera.rotation[1];
    float yaw = camera.rotation[2];
    return {sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(pitch) * cosf(yaw)};
}
Vector3f World::get_camera_up() const {
    float sp = sinf(camera.rotation[1]);
    float cp = cosf(camera.rotation[1]);
    float cr = cosf(camera.rotation[0]);
    float sr = sinf(camera.rotation[0]);
    float sy = sinf(camera.rotation[2]);
    float cy = cosf(camera.rotation[2]);

    return {-sp * sy * cr - sr * cy, cp * cr, sp * cr * cy - sr * sy};
}
Vector3f World::get_screen_point_oritation(Vector2f screen_xy) const {
    float w_w = float(window_width);
    float w_h = float(window_height);
    Vector3f camera_up = get_camera_up();
    Vector3f camera_forward = get_camera_oritation();
    Vector3f camera_right = camera_forward.cross(camera_up);
    float tan_fov = std::tan(camera.fov / 2);

    Vector3f ori = get_camera_oritation() + camera_right * ((screen_xy.x / w_w - 0.5f) * tan_fov * w_w / w_h * 2.0f) +
                   camera_up * ((0.5f - screen_xy.y / w_h) * tan_fov * 2.0f);

    return ori.normalize();
}
void World::init_start_scene() {
    {
        CameraDesc &desc = this->set_camera();
        desc.position = {0.0f, 0.0f, 0.0f};
        desc.rotation = {0.0f, 0.0f, 0.0f};
        desc.fov = 1.57f;
        desc.near_z = 1.0f;
        desc.far_z = 1000.0f;
    }
    // {
    //     create_object(GObjectDesc{
    //         {0.0f, 0.0f, -10.0f},
    //         {0.0f, 0.0f, 0.0f},
    //         {1.0f, 1.0f, 1.0f},
    //         {GameObjectPartDesc{"cube", "default"}}
    //     });
    // }
    // {
    //     create_object(GObjectDesc{
    //         {0.0f, 10.0f, -10.0f},
    //         {0.0f, 0.0f, 0.0f},
    //         {5.0f, 5.0f, 5.0f},
    //         {GameObjectPartDesc{"plane", "default"}}
    //     });
    // }
    // {
    //     GObject &platform = this->objects.at(this->create_object());
    //     platform.set_parts().emplace_back(
    //         GameObjectPartDesc{"platform", "default"});
    //     platform.set_position({0.0, -2.0, -10.0});
    //     platform.set_scale({4.0, 0.1, 4.0});
    // }
}