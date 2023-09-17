#pragma once
#include <assert.h>
#include <cstdio>
#include <GL/glew.h>
#include <GL/glext.h>
#include <vector>
#include "../utils/GuidAlloctor.h"
#include "GObject.h"

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