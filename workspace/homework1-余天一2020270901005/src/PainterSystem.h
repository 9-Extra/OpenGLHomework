#pragma once
#include "GlobalRuntime.h"
#include <stack>

class PainterSystem : public ISystem {
private:
    bool is_mouse_left_down = false;
    Vector3f start_postion;
    std::stack<uint32_t> object_stack;
    enum Select { LINE, SQUARE, CIRCLE } select = LINE;

public:
    void init() override {
        runtime->menu.add_item("painter", [&](MenuManager::MenuBuilder& builder){
            builder.set_menu(object_stack.empty() ? (MF_DISABLED | MF_GRAYED) : 0 | MF_STRING, L"撤销", [&]{
                runtime->world.kill_object(object_stack.top());
                object_stack.pop();
            });
            builder.set_menu(MF_SEPARATOR, L"", []{});//分割线
            
            builder.set_menu( select == LINE ? MF_CHECKED : 0 | MF_STRING, L"线段", [&]{
                if (!is_mouse_left_down) select = LINE;
            });
            builder.set_menu( select == SQUARE ? MF_CHECKED : 0 | MF_STRING, L"矩形", [&]{
                if (!is_mouse_left_down) select = SQUARE;
            });
            builder.set_menu( select == CIRCLE ? MF_CHECKED : 0 | MF_STRING, L"圆", [&]{
                if (!is_mouse_left_down) select = CIRCLE;
            });
        });
    }

    void tick() override {
        bool current_is_left_button_down = runtime->input.is_left_button_down();
        if (current_is_left_button_down) {
            World &world = runtime->world;
            Vector3f point_oritation = world.get_screen_point_oritation(
                runtime->input.get_mouse_position());
            Vector3f point =
                world.get_camera().position +
                point_oritation /
                    (point_oritation.dot(world.get_camera_oritation())) * 10.0f;

            if (!is_mouse_left_down) {
                // 左键点击
                start_postion = point;

                if (select == LINE) {
                    object_stack.push(world.create_object(
                        GObjectDesc{start_postion,
                                    {0.0f, 0.0f, 0.0f},
                                    {0.0f, 0.0f, 0.0f},
                                    {GameObjectPartDesc{
                                        "line", "default", GL_LINES,
                                        }}}));
                } else if (select == SQUARE) {
                    object_stack.push(world.create_object(
                        GObjectDesc{start_postion,
                                    {0.0f, 0.0f, 0.0f},
                                    {0.0f, 0.0f, 0.0f},
                                    {GameObjectPartDesc{
                                        "plane", "default", GL_TRIANGLES,
                                        }}}));
                } else if (select == CIRCLE){
                    object_stack.push(world.create_object(
                        GObjectDesc{start_postion,
                                    {0.0f, 0.0f, 0.0f},
                                    {0.0f, 0.0f, 0.0f},
                                    {GameObjectPartDesc{
                                        "circle", "default", GL_TRIANGLES,
                                        }}}));
                }

            } else {
                // 拖拽
                if (select == LINE) {
                    GObject& obj = world.objects.at(object_stack.top());
                    obj.set_scale(point - start_postion);
                } else if (select == SQUARE || select == CIRCLE) {
                    Vector3f max_corner{std::max(start_postion.x, point.x), std::max(start_postion.y, point.y), std::max(start_postion.z, point.z)};
                    Vector3f min_corner{std::min(start_postion.x, point.x), std::min(start_postion.y, point.y), std::min(start_postion.z, point.z)};
                    GObject& obj = world.objects.at(object_stack.top());
                    obj.set_scale(max_corner - min_corner);
                }
            }
        }

        if (is_mouse_left_down && !current_is_left_button_down) {
            // 左键松开
        }

        is_mouse_left_down = current_is_left_button_down;
    }

    ~PainterSystem() {
        runtime->menu.remove_item("painter");
    }
};
