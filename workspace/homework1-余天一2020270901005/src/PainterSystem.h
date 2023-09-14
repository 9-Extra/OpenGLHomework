#pragma once
#include "GlobalRuntime.h"

class PainterSystem: public ISystem{
private:
    bool is_mouse_left_down = false;
    Vector3f start_postion;
    uint32_t object_id;
    enum Select{
        LINE,
        SQUARE,
        CIRCLE
    } select = LINE;
public:
    void tick() override{
        bool current_is_left_button_down = runtime->input.is_left_button_down();
        if (current_is_left_button_down){
            if (!is_mouse_left_down){
                //左键点击
                World& world = runtime->world;
                Vector3f point_oritation = world.get_screen_point_oritation(runtime->input.get_mouse_position());
                start_postion = world.get_camera().position + point_oritation / (point_oritation.dot(world.get_camera_oritation())) * 10.0f;
                object_id = world.create_object(GObjectDesc{
                    start_postion,
                    point_oritation,
                    {1.0f, 1.0f, 1.0f},
                    {GameObjectPartDesc{
                        "cube", "default"
                    }}
                });
            } else {
                //拖拽
                World& world = runtime->world;
                Vector3f point_oritation = world.get_screen_point_oritation(runtime->input.get_mouse_position());
                Vector3f end_position = world.get_camera().position + point_oritation / (point_oritation.dot(world.get_camera_oritation()));
            }
        }
      
        if (is_mouse_left_down && !current_is_left_button_down){
            //左键松开
        }

        is_mouse_left_down = current_is_left_button_down;
    }
};


