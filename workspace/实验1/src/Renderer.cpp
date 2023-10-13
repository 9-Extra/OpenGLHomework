#include "Renderer.h"
#include "Simgui.h"

Renderer renderer; // global renderer

void Renderer::render() {
    if (!is_camera_updated) {
        main_camera = Camera();
        std::cout << "没有设置相机" << std::endl;
    }
       
    // 清空后缓冲区
    glDrawBuffer(GL_BACK); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    lambertian_pass->run();
    single_color_pass->run();
    
    //skybox_pass->run();
    // pickup_pass->run();

    lambertian_pass->reset();
    single_color_pass->reset();
    pointlights.clear();    
    
    SimGui::render();

    glutSwapBuffers(); // 渲染完毕，交换缓冲区，显示新一帧的画面
    checkError();

    is_camera_updated = false;
}
