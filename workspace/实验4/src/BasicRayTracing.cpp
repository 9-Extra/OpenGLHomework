#include "glmath.h"
#include <GL/glew.h>
#include <GL/glut.h>
#include "scene.h"

#include <iostream>
#include <vector>

using namespace std;

//---------------------------
// 用来初始化顶点着色器与片段着色器的类，其构造函数中生成了VertexArray与Buffer缓冲。
class FullScreenTexturedQuad {
    // texture id
    unsigned int textureId = 0;

public:
    FullScreenTexturedQuad(uint32_t, uint32_t) {
        // 启用二维纹理
        glEnable(GL_TEXTURE_2D);
        // 生成纹理标识符
        glGenTextures(1, &textureId);
        // 绑定纹理
        glBindTexture(GL_TEXTURE_2D, textureId);
        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // sampling
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // 加载纹理（光线追踪计算结果image作为纹理）
    void LoadTexture(vector<vec4> &image) {
        // 绑定纹理
        glBindTexture(GL_TEXTURE_2D, textureId);
        // 加载纹理（光线追踪计算颜色image）到纹理内存（to GPU）
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, &image[0]); // To GPU
    }
};

FullScreenTexturedQuad *fullScreenTexturedQuad;

//---------------------------
// 整个函数的初始化，设定视窗、初始化场景、初始化着色器，并创建gpu进程
void onInitialization() {
    // 设置视口变换
    glViewport(0, 0, windowWidth, windowHeight);

    // 建立场景
    scene.build();

    // 创建对象，初始化纹理
    fullScreenTexturedQuad = new FullScreenTexturedQuad(windowWidth, windowHeight);
}

long timebase = 0;
long time_last_frame;
void showFrame() {
    static long frame = 0;
    // 计算帧率
    frame++;
    long time = glutGet(GLUT_ELAPSED_TIME);
    time_last_frame = time;

    if (time - timebase > 1000) {
        float fps = frame * 1000.0 / (time - timebase);
        timebase = time;
        frame = 0;

        cout << "FPS:" << fps << endl;
    }
}

//---------------------------------------------
// 显示函数。
//   调用render求取光线追踪的结果，存到image中，
//   然后用LoadTexture更新当前帧的纹理，最后绘制出来
void onDisplay() {
    showFrame();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vector<vec4> image(windowWidth * windowHeight);

    // 场景绘制（通过光线追踪计算所有像素值，保存在image中）
    scene.render(image);

    // 使用这种方法的话画面不能缩放
    // glDrawPixels(windowWidth, windowHeight, GL_RGBA, GL_FLOAT, &image[0]);

    // 把光线追踪计算的image作为场景纹理
    fullScreenTexturedQuad->LoadTexture(image);

    // 绘制纹理
    glBegin(GL_POLYGON);
    // 设置纹理坐标与顶点坐标
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, -1.0, 0.0);

    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.0, 1.0, 0.0);

    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, 1.0, 0.0);

    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.0, -1.0, 0.0);
    glEnd();

    glutSwapBuffers();
}

void onReshape(int w, int h) {
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h)
        glOrtho(-1.0, 1.0, -1.0 * (GLfloat)h / (GLfloat)w, 1.0 * (GLfloat)h / (GLfloat)w, -10.0, 10.0);
    else
        glOrtho(-1.0 * (GLfloat)w / (GLfloat)h, 1.0 * (GLfloat)w / (GLfloat)h, -1.0, 1.0, -10.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    timebase = glutGet(GLUT_ELAPSED_TIME);
}

void onIdle() {
    // 视点旋转
    long time = glutGet(GLUT_ELAPSED_TIME);
    scene.animate(0.001f * (time - time_last_frame));

    glutPostRedisplay();
}

void onKeyboard(unsigned char key, int , int) {
    if (key == '1')
        glutIdleFunc(onIdle);
    if (key == '2')
        glutIdleFunc(NULL);

    if (key == 's') {
        long time = glutGet(GLUT_ELAPSED_TIME);
        scene.zoomInOut(0.05f * (time - time_last_frame));
    }

    if (key == 'w') {
        long time = glutGet(GLUT_ELAPSED_TIME);
        scene.zoomInOut(-0.05f * (time - time_last_frame));
    }
}

//-----------------------------------------
// Entry point of the application
int main(int argc, char *argv[]) {
    // Initialize GLUT, Glew and OpenGL
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight); // Application window is initially of resolution 600x600
    glutInitWindowPosition(100, 100);              // Relative location of the application window
    glutCreateWindow("Ray Tracing");

    glewExperimental = true;
    glewInit();
    // Initialize this program and create shaders
    onInitialization();

    // Register event handlers
    glutReshapeFunc(onReshape);
    glutDisplayFunc(onDisplay);
    glutIdleFunc(onIdle);
    // 键盘
    glutKeyboardFunc(onKeyboard);

    glutMainLoop();

    return 1;
}