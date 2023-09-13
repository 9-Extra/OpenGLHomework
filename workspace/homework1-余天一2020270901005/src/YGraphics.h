#pragma once
#include <gl/glew.h>
#include <cstdio>
#include <chrono>
#include <assert.h>
#include <vector>
#include "CGmath.h"

inline void checkError()
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR)
    {
        fprintf(stderr, "GL error 0x%X: %s\n", error, gluErrorString(error));
    }
}

class Clock
{
private:
    std::chrono::time_point<std::chrono::steady_clock> now;

public:
    Clock()
        : now(std::chrono::steady_clock::now())
    {
    }

    float update()
    {
        using namespace std::chrono;
        float delta = duration_cast<duration<float, std::milli>>(steady_clock::now() - now).count();
        now = std::chrono::steady_clock::now();
        return delta;
    }
};

static unsigned int calculate_fps(float delta_time){
    const float ratio = 0.1f;
    static float avarage_frame_time = std::nan("");

    if (std::isnormal(avarage_frame_time)){
        avarage_frame_time = avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time); 
}

struct Vertex
{
    Vector3f position;
    Color color;
};

class GraphicsObject
{
private:
    const std::vector<Vertex> vertices;
    const std::vector<unsigned int> indices;
    Matrix transformation;

public:
    Vector3f position{0.0, 0.0, 0.0};
    Vector3f rotation{0.0, 0.0, 0.0};//zxy顺序
    Vector3f scale{1.0,1.0,1.0};

    GraphicsObject(const std::vector<Vertex> &&vertices, const std::vector<unsigned int> &&indices)
        : vertices(vertices), indices(indices)
    {
        assert(indices.size() % 3 == 0);
    }

    void update()
    {
        transformation = 
        Matrix::translate(position[0],position[1],position[2]) * 
        Matrix::rotate(rotation[0],rotation[1],rotation[2]) * 
        Matrix::scale(scale[0],scale[1],scale[2]);
    }

    void draw_polygon()
    {
        glPushMatrix();
        glMultMatrixf(transformation.transpose().data());
        glBegin(GL_TRIANGLES);
        for (unsigned int i : indices)
        {
            const Vertex &v = vertices[i];
            glColor3fv(v.color.data());
            glVertex3fv(v.position.data());
        }
        glEnd();
        glPopMatrix();
    }

    void draw_wireframe(Color color)
    {
        glPushMatrix();
        glMultMatrixf(transformation.transpose().data());
        glColor3fv(color.data());
        size_t indices_size = indices.size();
        for(unsigned int i = 0;i < indices_size;i += 3){
            glBegin(GL_LINE_LOOP);
            glVertex3fv(vertices[indices[i]].position.data());
            glVertex3fv(vertices[indices[i + 1]].position.data());
            glVertex3fv(vertices[indices[i + 2]].position.data());
            glEnd();
        }
        glPopMatrix();
    }
};

class Camera
{
    friend class Scene;

private:
    Matrix view_transformation = Matrix::identity();
    
    Camera(){}

    void update(){
        view_transformation = Matrix::rotate(rotation[0],rotation[1],rotation[2]).transpose() * Matrix::translate(-position[0],-position[1],-position[2]);
    }
public:
    Vector3f position{0.0,0.0,0.0};
    Vector3f rotation{0.0,0.0,0.0};//zxy

    //获取目视方向
    Vector3f get_orientation() const{
        float pitch = rotation[1];
        float yaw = rotation[2];
        return {sinf(yaw)*cosf(pitch), sinf(pitch), -cosf(pitch)*cosf(yaw)};
    }
    
};

inline Matrix compute_perspective_matrix(float ratio, float fov, float near_z,
                                  float far_z) {
    float SinFov = std::sin(fov * 0.5f);
    float CosFov = std::cos(fov * 0.5f);

    float Height = CosFov / SinFov;
    float Width = Height / ratio;

    return Matrix{{Width, 0.0f, 0.0f, 0.0f, 0.0f, Height, 0.0f, 0.0f, 0.0f,
                   0.0f, -far_z / (far_z - near_z), -1.0f, 0.0f, 0.0f,
                   -far_z * near_z / (far_z - near_z), 0.0f}};
}

class Scene
{
public:
    Camera camera;
    std::vector<GraphicsObject> objects;
    Scene(){}

    void update()
    {
        camera.update();
        for (GraphicsObject& obj: objects){
            obj.update();
        }
    }

    void draw_polygon()
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(camera.view_transformation.transpose().data());
        for (GraphicsObject& obj: objects){
            obj.draw_polygon();
        }
    }

    void draw_wireframe()
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(camera.view_transformation.transpose().data());
        for (GraphicsObject& obj: objects){
            obj.draw_wireframe(Color{1.0,1.0,1.0});
        }
    }
};