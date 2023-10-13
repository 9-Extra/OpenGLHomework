#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "CGmath.h"

namespace SimGui {

struct Rect{
    uint32_t x, y;
    uint32_t w = 0, h = 0;

    bool contains(uint32_t x, uint32_t y) const {
        return x >= this->x && x < this->x + this->w && y >= this->y && y < this->y + this->h;
    }
};

class Widget;
struct GlobalContext final{
    // 存储该帧更新的widget, 先尝试从widgets_storage查找，找不到就新建，每帧结束时将所有内容移至storage中
    std::unordered_map<std::string, std::unique_ptr<Widget>> widgets; 
    std::unordered_map<std::string, std::unique_ptr<Widget>> widgets_storage;// 存储之前的所有widget
    uint32_t screen_width, screen_height;
    uint32_t mouse_x, mouse_y;
    int mouse_delta_x, mouse_delta_y;
    void *font_style;

    std::string seleted_widget_name;

    bool is_mouse_pressed = false;
    bool is_mouse_clicked = false;
};

extern GlobalContext g_context;

class Widget {
public:
    Widget(const std::string& name, const Rect& rect): rect(rect), name(name) {
        assert(!this->name.empty());
    }
    virtual void render() = 0;
    virtual void on_dragging(int dx, int dy) {}
    virtual void on_clicked(uint32_t x, uint32_t y) {}

    bool operator==(const Widget& other) const {
        return name == other.name;
    }
    Rect rect;
    const std::string name;
protected:
    friend void render();
    bool is_hovered() const {
        return rect.contains(g_context.mouse_x, g_context.mouse_y);
    }
    bool is_selected() const {
        return g_context.seleted_widget_name == name;
    }
};

void init();
inline void set_screen_size(uint32_t screen_width, uint32_t screen_height){
    g_context.screen_width = screen_width;
    g_context.screen_height = screen_height;
}

inline void update_mouse_pos(uint32_t x, uint32_t y){
    g_context.mouse_delta_x = int(x) - int(g_context.mouse_x);
    g_context.mouse_delta_y = int(y) - int(g_context.mouse_y);
    g_context.mouse_x = x;
    g_context.mouse_y = y;
}
inline void do_mouse_down(){
    g_context.is_mouse_clicked = true;
    g_context.is_mouse_pressed = true;
}

inline void do_mouse_release(){
    g_context.is_mouse_pressed = false;
    g_context.seleted_widget_name.clear();
}

bool show_cell_float(const std::string& name, const Rect& r, float* f, float sensitive=0.01f, float minium=0.0f, float maxium=1.0f, const char *format="%.2f");
void show_text(const std::string& name, const Rect& r, const char *str);

void render();

}