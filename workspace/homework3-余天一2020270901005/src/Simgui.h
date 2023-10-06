#pragma once

#include <vector>
#include <string>

namespace SimGui {

struct RenderBackbone final{




};

struct Widget{


};

struct Window{
    std::string title;
    std::vector<Widget> widgets;

    uint32_t width;
    uint32_t height;
};

struct GlobalContext final{
    std::vector<Window> windows;


};

void init();
void start_frame();
void test_window();
void end_frame();
void clear();

}