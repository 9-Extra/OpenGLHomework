#include "Simgui.h"

#include <GL/glew.h>
#include <GL/glut.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
namespace SimGui {

GlobalContext g_context;

Vector2f screen_axis_map(uint32_t x, uint32_t y) {
    return Vector2f(float(x) / float(g_context.screen_width) * 2 - 1, 1 - float(y) / float(g_context.screen_height) * 2);
}

void setfont(const char* name, int size)
{
    g_context.font_style = GLUT_BITMAP_HELVETICA_10;
    if (strcmp(name, "helvetica") == 0) {
        if (size == 12) 
            g_context.font_style = GLUT_BITMAP_HELVETICA_12;
        else if (size == 18)
            g_context.font_style = GLUT_BITMAP_HELVETICA_18;
    } else if (strcmp(name, "times roman") == 0) {
        g_context.font_style = GLUT_BITMAP_TIMES_ROMAN_10;
        if (size == 24)
            g_context.font_style = GLUT_BITMAP_TIMES_ROMAN_24;
    } else if (strcmp(name, "8x13") == 0) {
        g_context.font_style = GLUT_BITMAP_8_BY_13;
    } else if (strcmp(name, "9x15") == 0) {
        g_context.font_style = GLUT_BITMAP_9_BY_15;
    }
}

void drawstr(GLuint x, GLuint y, const char *string) { 
    Vector2f pos = screen_axis_map(x, y + 20); // 加点偏移方便点
    glRasterPos2fv(pos.v);
    for (const char* s = string; *s; s++){
        glutBitmapCharacter(g_context.font_style, *s);
    }
}

// 用于格式控制
void init(){
    setfont("helvetica", 18);
}

void render(){
    glUseProgram(0);
    
    if (auto it = g_context.widgets.find(g_context.seleted_widget_name);it != g_context.widgets.end()) {
        if (g_context.is_mouse_pressed){
            it->second->on_dragging(g_context.mouse_delta_x, g_context.mouse_delta_y);
        }
    } else {
        g_context.seleted_widget_name.clear();
    }

    for(auto& [name, widget]: g_context.widgets){
        if (widget->is_hovered() && g_context.is_mouse_clicked){
            widget->on_clicked(g_context.mouse_x, g_context.mouse_y);
            if (g_context.seleted_widget_name.empty()){
                g_context.seleted_widget_name = widget->name;
            }
        }
        widget->render();
    }
    
    g_context.widgets_storage.merge(std::move(g_context.widgets));
    g_context.widgets.clear();

    g_context.mouse_delta_x = 0;
    g_context.mouse_delta_y = 0;
    g_context.is_mouse_clicked = false;
}

bool show_cell_float(const std::string& name, const Rect& r, float* f, float sensitive, float minium, float maxium, const char *format){
    assert(maxium >= minium);
    struct FloatCell : public Widget{
        FloatCell(const std::string& name, const Rect& rect, const char *format, float value) : Widget(name, rect), format(format), value(value){}
        const char *format;
        float value; // 暂时保存值用于显示，更新值也暂存于此，于下一帧写入
        int delta = 0; // 记录拖动幅度
        virtual void render(){
            char buffer[256];
            sprintf_s(buffer, format, value);
            if (is_selected()){
                glColor3ub(255, 0, 0);
            } else {
                if (is_hovered()){
                    glColor3ub(0, 255, 0);
                } else {
                    glColor3ub(255, 255, 255);
                }
            }
            drawstr(rect.x, rect.y, buffer);
        }

        virtual void on_dragging(int dx, int dy){
            delta = -dy;
        }
    };

    bool is_changed = false;
    if (auto it = g_context.widgets_storage.find(name); it != g_context.widgets_storage.end()){
        FloatCell* cell = static_cast<FloatCell*>(it->second.get());
        assert(cell);
        cell->rect = r;
        cell->format = format;

        if (cell->delta != 0){
            cell->value = std::clamp<float>(cell->value + cell->delta * sensitive * (maxium - minium), minium, maxium);
            *f = cell->value;
            cell->delta = 0;
            is_changed = true;
        } else {
            cell->value = std::clamp<float>(*f, minium, maxium);
        }
        g_context.widgets.insert(g_context.widgets_storage.extract(it));
    } else {
        g_context.widgets.insert_or_assign(name, std::make_unique<FloatCell>(name, r, format, *f));
    }

    return is_changed;
}
void show_text(const std::string& name, const Rect& r, const char *str){
    struct StrCell : public Widget{
        StrCell(const std::string& name,const Rect& rect, const char *str) : Widget(name, rect), str(str){}
        const char *str;
        virtual void render(){
            glColor3ub(255, 255, 255);
            drawstr(rect.x, rect.y, str);
        }
    };

    if (auto it = g_context.widgets_storage.find(name); it != g_context.widgets_storage.end()){
        StrCell* cell = static_cast<StrCell*>(it->second.get());
        assert(cell);
        cell->rect = r;
        cell->str = str;
        g_context.widgets.insert(g_context.widgets_storage.extract(it));
    } else {
        g_context.widgets.insert_or_assign(name, std::make_unique<StrCell>(name, r, str));
    }

}

} // namespace SimGui