#include "Simgui.h"

namespace SimGui {

void begin_window(const std::string& title){}
void show_bulletText(const char* str, ...){}
void show_text(const char* str) {}
void show_slider(const char* str, float* value, float min, float max, const char* format = "%.3f"){}
bool show_button(const char* str) { return false; }
void end_window() {}

void test_window(){
    begin_window("test");
    float x, y, z;
    show_bulletText("%.1f,%.1f,%.1f", &x, &y, &z);
    show_text("Fov");
    float fov, yaw, pitch, roll;
    show_slider("fov", &fov,10.0f,170.0f);
    show_text("Rotation");
    show_slider("yaw", &yaw,-180.0f,180.0f);
    show_slider("pitch", &pitch, -180.0f, 180.0f);
    show_slider("roll", &roll, -180.0f, 180.0f);
    if (show_button("Reset")) {
        yaw = 0.0f;
        pitch = 0.0f;
        roll = 0.0f;
    }
    end_window();//Camera
}

void init(){

}
void start_frame(){

}

void end_frame(){

}
void clear(){


}


}