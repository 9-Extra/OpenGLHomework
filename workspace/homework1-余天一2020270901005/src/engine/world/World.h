#pragma once
#include <assert.h>
#include <cstdio>
#include <GL/glew.h>
#include <GL/glext.h>
#include <vector>
#include "GObject.h"

class World {
public:
    uint32_t window_width,window_height; 
    World(Renderer& renderer): renderer(renderer) {
        init_start_scene();
        update_swap_data();
    }

    template<class T = GObject, class... ARGS>
    std::shared_ptr<T> create_object(ARGS&&... args){
        std::shared_ptr<T> g_object = std::make_shared<T>(std::forward<ARGS>(args)...);
        assert(dynamic_cast<GObject*>(g_object.get()));
        objects.emplace(g_object.get(), g_object);
        return g_object;
    }

    void register_object(std::shared_ptr<GObject> g_object){
        assert(objects.find(g_object.get()) == objects.end());//不可重复添加
        objects.emplace(g_object.get(), g_object);
    }

    void kill_object(std::shared_ptr<GObject> g_object){
        size_t i = objects.erase(g_object.get());
        assert(i != 0);//试图删除不存在的object
        renderer.get_logic_swap_data().delete_game_object((size_t)g_object.get());
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
    CameraDesc camera;
    bool is_camera_dirty{false};
    Renderer& renderer;
    std::unordered_map<GObject*, std::shared_ptr<GObject>> objects;

    void init_start_scene();
};