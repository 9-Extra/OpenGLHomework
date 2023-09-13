#pragma once
#include "CGmath.h"
#include <assert.h>
#include <cstdio>
#include <GL/glew.h>
#include <GL/glext.h>
#include <vector>


inline void checkError() {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "GL error 0x%X: %s\n", error, gluErrorString(error));
    }
}

static unsigned int calculate_fps(float delta_time) {
    const float ratio = 0.1f;
    static float avarage_frame_time = std::nan("");

    if (std::isnormal(avarage_frame_time)) {
        avarage_frame_time =
            avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time);
}

struct Vertex {
    Vector3f position;
    Color color;
};

class GraphicsObject {
private:
    const std::vector<Vertex> vertices;
    const std::vector<unsigned int> indices;
    Matrix transformation;

    void update(){
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf((
            Matrix::translate(position[0], position[1], position[2]) *
            Matrix::rotate(rotation[0], rotation[1], rotation[2]) *
            Matrix::scale(scale[0], scale[1], scale[2])
        ).transpose().data()
        );
    }

public:
    Vector3f position{0.0, 0.0, 0.0};
    Vector3f rotation{0.0, 0.0, 0.0}; // zxy顺序
    Vector3f scale{1.0, 1.0, 1.0};

    GraphicsObject(const std::vector<Vertex> &&vertices,
                   const std::vector<unsigned int> &&indices)
        : vertices(vertices), indices(indices) {
        assert(indices.size() % 3 == 0);
    }

    void draw_polygon(){
        update();
        glBegin(GL_TRIANGLES);
        for (unsigned int i : indices) {
            const Vertex &v = vertices[i];
            glColor3fv(v.color.data());
            glVertex3fv(v.position.data());
        }
        glEnd();
    }

    void draw_wireframe(Color color) {
        update();
        glColor3fv(color.data());
        size_t indices_size = indices.size();
        for (unsigned int i = 0; i < indices_size; i += 3) {
            glBegin(GL_LINE_LOOP);
            glVertex3fv(vertices[indices[i]].position.data());
            glVertex3fv(vertices[indices[i + 1]].position.data());
            glVertex3fv(vertices[indices[i + 2]].position.data());
            glEnd();
        }
    }
};

inline Matrix compute_perspective_matrix(float ratio, float fov, float near_z,
                                         float far_z) {
    float SinFov = std::sin(fov * 0.5f);
    float CosFov = std::cos(fov * 0.5f);

    float Height = CosFov / SinFov;
    float Width = Height / ratio;

    return Matrix{Width, 0.0f, 0.0f, 0.0f, 0.0f, Height, 0.0f, 0.0f, 0.0f,
                   0.0f, -far_z / (far_z - near_z), -1.0f, 0.0f, 0.0f,
                   -far_z * near_z / (far_z - near_z), 0.0f}.transpose();
}

class Camera {
public:
    Camera() {}
    
    float fov = 1.57f, far_z = 1000.0f, near_z = 0.1f;
    Vector3f position{0.0, 0.0, 0.0};
    Vector3f rotation{0.0, 0.0, 0.0}; // zxy

    // 获取目视方向
    Vector3f get_orientation() const {
        float pitch = rotation[1];
        float yaw = rotation[2];
        return {sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(pitch) * cosf(yaw)};
    }

    void update(float ratio) {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf((
            compute_perspective_matrix(ratio, fov, near_z, far_z) *
            Matrix::rotate(rotation).transpose() *
            Matrix::translate(-position)
        ).transpose().data());
    }
};
class Scene {
public:
    Camera camera;
    std::vector<GraphicsObject> objects;
    Scene() {}

    void draw_polygon() {
        for (GraphicsObject &obj : objects) {
            obj.draw_polygon();
        }
    }

    void draw_wireframe() {
        for (GraphicsObject &obj : objects) {
            obj.draw_wireframe(Color{1.0, 1.0, 1.0});
        }
    }
};