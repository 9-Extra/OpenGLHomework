#pragma once
#include <gl/glew.h>
#include <GL/glut.h>
#include <cstdio>
#include <chrono>
#include <assert.h>
#include <vector>
#include <cmath>

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

struct Color
{
    GLfloat r, g, b;

    inline const GLfloat *data() const
    {
        return (GLfloat *)this;
    }

    inline bool operator==(const Color &ps)
    {
        return r == ps.r && g == ps.g && b == ps.b;
    }

    inline bool operator!=(const Color &ps)
    {
        return !(*this == ps);
    }
};

//即1.0 / sqrt(x)
inline float InvSqrt(float x)
{
    float xhalf = 0.5f * x;
    int i = *(int *)&x;             // get bits for floating value
    i = 0x5f3759df - (i >> 1);      // gives initial guess y0
    x = *(float *)&i;               // convert bits back to float
    x = x * (1.5f - xhalf * x * x); // Newton step, repeating increases accuracy
    return x;
}

struct Vector3f{
    union{
        struct
        {
            float x,y,z;
        };
        float v[3];
    };

    const GLfloat *data() const
    {
        return (GLfloat *)v;
    }

    inline float operator [](const unsigned int i) const{
        return v[i];
    }

    inline Vector3f operator +(const Vector3f b) const{
        return Vector3f{x + b.x,y + b.y,z + b.z};
    }

    inline Vector3f operator+=(const Vector3f b){
        return *this = *this + b;
    }

    inline Vector3f operator*(const float n){
        return {x * n,y * n,z * n};
    }

    inline float dot(const Vector3f b){
        return x * b.x + y * b.y + z * b.z;
    }

    inline Vector3f cross(const Vector3f b){
        return {this->y * b.z - this->z * b.y,this->x * b.z - this->z * b.y,this->x * b.y - this->y * b.x};
    }

    inline float square(){
        return this->dot(*this);
    }

    inline Vector3f normalize(){
        float inv_sqrt = InvSqrt(this->square());
        return *this * inv_sqrt;
    }
};

struct Matrix
{
    float m[4][4];

    GLfloat *data()
    {
        return (GLfloat *)m;
    }

    inline Matrix transpose(){
        Matrix r;
        for(unsigned int i = 0;i < 4;i++){
            for(unsigned int j = 0;j < 4;j++){
                r.m[i][j] = this->m[j][i];
            }
        }
        return r;
    }

    inline static Matrix identity()
    {
        Matrix m{
            {
                {1.0, 0.0, 0.0, 0.0},
                {0.0, 1.0, 0.0, 0.0},
                {0.0, 0.0, 1.0, 0.0},
                {0.0, 0.0, 0.0, 1.0},
            }};

        return m;
    }

    inline Matrix operator*(const Matrix &m)
    {
        Matrix r;
        for (unsigned int i = 0; i < 4; i++)
        {
            for (unsigned int j = 0; j < 4; j++)
            {
                r.m[i][j] = this->m[i][0] * m.m[0][j] + this->m[i][1] * m.m[1][j] + this->m[i][2] * m.m[2][j] + this->m[i][3] * m.m[3][j];
            }
        }
        return r;
    }
    //沿z轴顺时针旋转roll，沿x轴顺时针旋转pitch，沿y轴顺时针旋转yaw
    inline static Matrix rotate(float roll, float pitch, float yaw)
    {
        float s_p = sin(pitch), c_p = cos(pitch);
        float s_r = sin(roll), c_r = cos(roll);
        float s_y = sin(yaw), c_y = cos(yaw);
        Matrix m{
            {{-s_p * s_r * s_y + c_r * c_y, -s_p * s_y * c_r - s_r * c_y, -s_y * c_p, 0.0},
             {s_r * c_p, c_p * c_r, -s_p, 0.0},
             {s_p * s_r * c_y + s_y * c_r, s_p * c_r * c_y - s_r * s_y, c_p * c_y, 0.0},
             {0.0, 0.0, 0.0, 1.0}}};
        return m;
    }

    inline static Matrix translate(float x, float y, float z)
    {
        Matrix m{{
            {1.0, 0.0, 0.0, x},
            {0.0, 1.0, 0.0, y},
            {0.0, 0.0, 1.0, z},
            {0.0, 0.0, 0.0, 1.0},
        }};
        return m;
    }

    inline static Matrix scale(float x,float y,float z){
        Matrix m{
            {
                {x, 0.0, 0.0, 0.0},
                {0.0, y, 0.0, 0.0},
                {0.0, 0.0, z, 0.0},
                {0.0, 0.0, 0.0, 1.0},
            }};

        return m;
    }
};

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