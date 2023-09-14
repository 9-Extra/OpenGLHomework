#pragma once
#include "GlobalRuntime.h"

class PainterSystem : public ISystem {
private:
    bool is_mouse_left_down = false;
    Vector3f start_postion;
    uint32_t object_id;
    enum Select { LINE, SQUARE, CIRCLE } select = LINE;
    std::vector<uint32_t> menu_ids;

public:
    void init() override {
        menu_ids.push_back(runtime->display.append_menu_item(
            MF_CHECKED | MF_STRING, L"画直线", [&](uint32_t id) {
                if (!runtime->input.is_right_button_down()) {
                    select = LINE;
                    runtime->display.modify_menu_item(
                        menu_ids[0], MF_CHECKED | MF_STRING, L"画直线");
                    runtime->display.modify_menu_item(menu_ids[1], MF_STRING,
                                                      L"画方形");
                    runtime->display.modify_menu_item(menu_ids[2], MF_STRING,
                                                      L"画圆");
                }
            }));
        menu_ids.push_back(runtime->display.append_menu_item(
            MF_STRING, L"画方形", [&](uint32_t id) {
                if (!runtime->input.is_right_button_down()) {
                    select = SQUARE;
                    runtime->display.modify_menu_item(menu_ids[0], MF_STRING,
                                                      L"画直线");
                    runtime->display.modify_menu_item(
                        menu_ids[1], MF_CHECKED | MF_STRING, L"画方形");
                    runtime->display.modify_menu_item(menu_ids[2], MF_STRING,
                                                      L"画圆");
                }
            }));
        menu_ids.push_back(runtime->display.append_menu_item(
            MF_STRING, L"画圆", [&](uint32_t id) {
                if (!runtime->input.is_right_button_down()) {
                    select = CIRCLE;
                    runtime->display.modify_menu_item(menu_ids[0], MF_STRING,
                                                      L"画直线");
                    runtime->display.modify_menu_item(menu_ids[1], MF_STRING,
                                                      L"画方形");
                    runtime->display.modify_menu_item(
                        menu_ids[2], MF_CHECKED | MF_STRING, L"画圆");
                }
            }));
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
                    object_id = world.create_object(
                        GObjectDesc{start_postion,
                                    {0.0f, 0.0f, 0.0f},
                                    {1.0f, 1.0f, 1.0f},
                                    {GameObjectPartDesc{
                                        "line", "default", GL_LINES,
                                        Matrix::scale({0.0f, 0.0f, 0.0f})}}});
                } else if (select == SQUARE) {
                    object_id = world.create_object(
                        GObjectDesc{start_postion,
                                    {0.0f, 0.0f, 0.0f},
                                    {1.0f, 1.0f, 1.0f},
                                    {GameObjectPartDesc{
                                        "plane", "default", GL_TRIANGLES,
                                        Matrix::scale({0.0f, 0.0f, 0.0f})}}});
                }

            } else {
                // 拖拽
                if (select == LINE){
                    Matrix transform = Matrix::scale(point - start_postion);
                    world.objects.at(object_id).set_parts()[0].model_matrix =
                    transform;
                } else if (select == SQUARE){
                    Vector3f max_corner = point;
                    Vector3f min_corner = start_postion;
                    if (max_corner.x < min_corner.x){
                        std::swap(max_corner.x, min_corner.x);
                    }
                    if (max_corner.y < min_corner.y){
                        std::swap(max_corner.y, min_corner.y);
                    }
                    if (max_corner.z < min_corner.z){
                        std::swap(max_corner.z, min_corner.z);
                    }
                    Matrix transform = Matrix::scale(max_corner - min_corner);
                    world.objects.at(object_id).set_parts()[0].model_matrix =
                    transform;
                }
            }
        }

        if (is_mouse_left_down && !current_is_left_button_down) {
            // 左键松开
        }

        is_mouse_left_down = current_is_left_button_down;
    }

    ~PainterSystem() {
        for (uint32_t id : menu_ids) {
            runtime->display.delete_menu_item(id);
        }
    }
};
