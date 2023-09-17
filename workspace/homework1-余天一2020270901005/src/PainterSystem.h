#pragma once
#include "engine/GlobalRuntime.h"
#include <stack>

class RotatingCircle: public GObject{
    using GObject::GObject;//继承构造函数

    virtual void tick() override{
        this->rotation.x += 0.001f * runtime->logic_clock.get_delta();
        mark_dirty();
    }
};

class PainterSystem : public ISystem {
private:
    bool is_mouse_left_down = false;
    Vector3f start_postion;
    float z_bias;
    std::stack<std::shared_ptr<GObject>> object_stack;
    enum Select { LINE = 0, SQUARE = 1, CIRCLE = 2 } select = LINE;

public:
    void init() override {
        // 设置目录
        runtime->menu.add_item("painter", [&](MenuManager::MenuBuilder &builder) {
            builder.set_menu(object_stack.empty() ? (MF_DISABLED | MF_GRAYED) : 0 | MF_STRING, L"撤销", [&] {
                runtime->world.kill_object(object_stack.top());
                object_stack.pop();
            });
            builder.set_menu(MF_SEPARATOR, L"", [] {}); // 分割线

            builder.set_menu(select == LINE ? MF_CHECKED : 0 | MF_STRING, L"线段", [&] {
                if (!is_mouse_left_down)
                    select = LINE;
            });
            builder.set_menu(select == SQUARE ? MF_CHECKED : 0 | MF_STRING, L"矩形", [&] {
                if (!is_mouse_left_down)
                    select = SQUARE;
            });
            builder.set_menu(select == CIRCLE ? MF_CHECKED : 0 | MF_STRING, L"圆", [&] {
                if (!is_mouse_left_down)
                    select = CIRCLE;
            });
        });
    }

    // 当点击时
    void on_click() {
        World &world = runtime->world;

        z_bias = 200.0f - object_stack.size() * 0.01f; // 使新绘制的图形离相机更近防止遮挡
        Vector3f point_oritation = world.get_screen_point_oritation(runtime->input.get_mouse_position());
        // 计算鼠标点击的点在世界坐标系中的位置
        Vector3f point = world.get_camera().position +
                         point_oritation / (point_oritation.dot(world.get_camera_oritation())) * z_bias;

        start_postion = point;

        static const std::string mesh_name[3] = {"line", "plane", "circle"};
        static const uint32_t topology[3] = {GL_LINES, GL_TRIANGLES, GL_TRIANGLES};
        GObjectDesc objdesc{start_postion,
                            {0.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 0.0f},
                            {GameObjectPartDesc{
                                mesh_name[select],
                                "default",
                                topology[select],
                            }}};
        if (select == CIRCLE){
            object_stack.push(world.create_object<RotatingCircle>(objdesc));
        } else {
            object_stack.push(world.create_object(objdesc));
        }
    }

    void on_drag() {
        World &world = runtime->world;
        if (object_stack.empty()) {
            return;
        }
        Vector3f point_oritation = world.get_screen_point_oritation(runtime->input.get_mouse_position());
        // 计算鼠标点击的点在世界坐标系中的位置
        Vector3f point = world.get_camera().position +
                         point_oritation / (point_oritation.dot(world.get_camera_oritation())) * z_bias;

        std::shared_ptr<GObject> obj = object_stack.top();
        if (select == LINE) {
            obj->scale = point - start_postion;
            obj->mark_dirty();
        } else if (select == SQUARE) {
            Vector3f dis = point - start_postion;
            // 修正为正数，防止翻转物体的朝向导致看不见
            dis.x = std::abs(dis.x);
            dis.y = std::abs(dis.y);
            dis.z = std::abs(dis.z);
            obj->scale = dis;
            obj->mark_dirty();
        } else if (select == CIRCLE) {
            float distance = (point - start_postion).length();
            obj->scale = {distance, distance, distance};
            obj->mark_dirty();
        }
    }

    void tick() override {
        bool current_is_left_button_down = runtime->input.is_left_button_down();
        if (current_is_left_button_down) {
            if (!is_mouse_left_down) {
                // 左键点击
                on_click();
            } else {
                // 拖拽
                on_drag();
            }
        }

        is_mouse_left_down = current_is_left_button_down;
    }

    ~PainterSystem() { runtime->menu.remove_item("painter"); }
};
