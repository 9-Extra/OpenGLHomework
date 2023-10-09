#pragma once

#include "Renderer.h"
#include "GObject.h"

class CpntCamera: public Component{
public:
    CpntCamera(float near_z = 1.0f, float far_z = 1000.0f, float fov = 1.57): near_z(near_z), far_z(far_z), fov(fov) {}
    float near_z, far_z, fov;

    // 将此相机设置为主相机
    void set_main_camera(){
        assert(get_owner() != nullptr);
        GObject& owner = *get_owner();
        renderer.active_camera = &owner;
    }

    virtual void tick() override {
        assert(get_owner() != nullptr);
        GObject& owner = *get_owner();
        if (renderer.active_camera == &owner){
            renderer.main_camera.far_z = far_z;
            renderer.main_camera.near_z = near_z;
            renderer.main_camera.fov = fov;
            renderer.main_camera.position = owner.transform.position;
            renderer.main_camera.rotation = owner.transform.rotation;

            renderer.is_camera_updated = true;
        }
    }
};

