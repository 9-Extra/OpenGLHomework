#include "YGraphics.h"
#include <memory>
#include <sstream>

struct GlobalRuntime {
    bool mouse_left_pressed = false, mouse_right_pressed = false;
    int mouse_last_x, mouse_last_y;

    bool render_frame = false;

    Scene scene;
    Clock clock;
};

std::unique_ptr<GlobalRuntime> runtime;

const std::vector<Vertex> cube_vertices = {
    {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}}, {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
    {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},   {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},
    {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},  {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
    {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},    {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}}};

const std::vector<Vertex> platform_vertices = {
    {{-1.0, -1.0, -1.0}, {0.5, 0.5, 0.5}}, {{1.0, -1.0, -1.0}, {0.5, 0.5, 0.5}},
    {{1.0, 1.0, -1.0}, {0.5, 0.5, 0.5}},   {{-1.0, 1.0, -1.0}, {0.5, 0.5, 0.5}},
    {{-1.0, -1.0, 1.0}, {0.5, 0.5, 0.5}},  {{1.0, -1.0, 1.0}, {0.5, 0.5, 0.5}},
    {{1.0, 1.0, 1.0}, {0.5, 0.5, 0.5}},    {{-1.0, 1.0, 1.0}, {0.5, 0.5, 0.5}}};

const std::vector<unsigned int> cube_indices = {
    1, 0, 3, 3, 2, 1, 3, 7, 6, 6, 2, 3, 7, 3, 0, 0, 4, 7,
    2, 6, 5, 5, 1, 2, 4, 5, 6, 6, 7, 4, 5, 4, 0, 0, 1, 5};

void init(void) {
    runtime = std::make_unique<GlobalRuntime>();

    GraphicsObject cube =
        GraphicsObject(std::move(cube_vertices), std::move(cube_indices));
    cube.position = {0.0, 0.0, -10.0};
    runtime->scene.objects.emplace_back(std::move(cube));

    GraphicsObject platform =
        GraphicsObject(std::move(platform_vertices), std::move(cube_indices));
    platform.scale = {4.0, 0.1, 4.0};
    platform.position = {0.0, -2.0, -10.0};
    runtime->scene.objects.emplace_back(std::move(platform));

    glClearColor(0.0, 0.0, 0.0, 0.0);
    // glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

Matrix compute_perspective_matrix(float ratio, float fov, float near_z,
                                  float far_z) {
    float SinFov = std::sin(fov * 0.5f);
    float CosFov = std::cos(fov * 0.5f);

    float Height = CosFov / SinFov;
    float Width = Height / ratio;

    return Matrix{{
        Width, 0.0f, 0.0f, 0.0f,
        0.0f, Height, 0.0f, 0.0f,
        0.0f, 0.0f, -far_z / (far_z - near_z), -1.0f,
        0.0f, 0.0f, -far_z * near_z / (far_z - near_z), 0.0f
    }};
}

void myReshape(GLsizei w, GLsizei h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(
        compute_perspective_matrix(float(w) / float(h), 90.0f / 180.0f * 3.14f, 1.0f, 1000.0f)
            .transpose()
            .data());
    // gluPerspective(90.0f, double(w) / double(h), 1.0, 1000.0);
}

void renderScene() {
    glutPostRedisplay();
    float delta = runtime->clock.update();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    runtime->scene.update();
    if (!runtime->render_frame) {
        runtime->scene.draw_polygon();
    } else {
        runtime->scene.draw_wireframe();
    }

    std::stringstream formatter;
    Vector3f position = runtime->scene.camera.position;
    Vector3f look = runtime->scene.camera.get_orientation();
    formatter << "FPS: " << calculate_fps(delta);
    formatter << "(" << position.x << ", " << position.y << ", " << position.z
              << ")   ";
    formatter << "(" << look.x << ", " << look.y << ", " << look.z << ")";
    glutSetWindowTitle(formatter.str().c_str());

    glFlush();

    glutSwapBuffers();

    checkError();
}

// 只在鼠标按下时启用
void mouseDrag(int x, int y) {
    // 左上角为(0,0)，右下角为(w,h)

    float dx = x - runtime->mouse_last_x, dy = y - runtime->mouse_last_y;
    runtime->mouse_last_x = x;
    runtime->mouse_last_y = y;

    if (runtime->mouse_left_pressed) {
        // 旋转图形
        const float s = 0.03;
        runtime->scene.objects[0].rotation += {0.0, dy * s, -dx * s};
    }

    if (runtime->mouse_right_pressed) {
        // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
        const float rotate_speed = 0.01;
        runtime->scene.camera.rotation.z += dx * rotate_speed;
        runtime->scene.camera.rotation.y -= dy * rotate_speed;
    }
}

void mouseClick(int button, int stat, int x, int y) {
    if (stat == GLUT_DOWN) {
        runtime->mouse_last_x = x;
        runtime->mouse_last_y = y;
    }

    if (button == GLUT_LEFT_BUTTON) {
        if (stat == GLUT_DOWN) {
            runtime->mouse_left_pressed = true;
        }
        if (stat == GLUT_UP) {
            runtime->mouse_left_pressed = false;
        }
    }
    if (button == GLUT_RIGHT_BUTTON) {
        if (stat == GLUT_DOWN) {
            runtime->mouse_right_pressed = true;
        }
        if (stat == GLUT_UP) {
            runtime->mouse_right_pressed = false;
        }
    }
}

void keyPress(int key, int x, int y);
void keyPressShort(unsigned char key, int x, int y) {
    keyPress(int(key), x, y);
}
void keyPress(int key, int x, int y) {
    printf("Get key %c\n", key);

    Scene &scene = runtime->scene;
    const float move_speed = 0.1;

    switch (key) {
    case 0x1b: // ESC
        glutDestroyWindow(glutGetWindow());
        break;
    case 'w': {
        Vector3f ori = scene.camera.get_orientation();
        ori.y = 0.0;
        scene.camera.position += ori.normalize() * move_speed;
        break;
    }

    case 's': {
        Vector3f ori = scene.camera.get_orientation();
        ori.y = 0.0;
        scene.camera.position += ori.normalize() * -move_speed;
        break;
    }

    case 'a': {
        Vector3f ori = scene.camera.get_orientation();
        ori = {ori.z, 0.0, -ori.x};
        scene.camera.position += ori.normalize() * move_speed;
        break;
    }

    case 'd': {
        Vector3f ori = scene.camera.get_orientation();
        ori = {ori.z, 0.0, -ori.x};
        scene.camera.position += ori.normalize() * -move_speed;
        break;
    }

    case 0x20: { // SPACE
        scene.camera.position.y += move_speed;
        break;
    }

    case 'f': {
        scene.camera.position.y -= move_speed;
        break;
    }

    case '1':
        runtime->render_frame = !runtime->render_frame;
        break;

    case '0':
        scene.camera.position = {0.0, 0.0, 0.0};
        scene.camera.rotation = {0.0, 0.0, 0.0};
        scene.objects[0].rotation = {0.0, 0.0, 0.0};
        break;

    default:
        break;
    }
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(50, 100);
    glutInitWindowSize(400, 300);
    glutCreateWindow("DrawCube");

    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseDrag);
    glutKeyboardFunc(keyPressShort);
    glutSpecialFunc(keyPress);

    glutReshapeFunc(myReshape);

    init();

    glutDisplayFunc(renderScene);

    runtime->clock.update();
    glutMainLoop();
}
