#pragma once

#include <GL/glew.h>
#include <GL/glut.h>

#include <string>
#include <iostream>
#include <assert.h>

inline void _check_error(const std::string &file, size_t line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "GL error 0x" << error << ": " << gluErrorString(error) << std::endl;
        std::cerr << "At: " << file << ':' << line << std::endl;
    }
}

#ifdef NDEBUG
#define checkError()
#else
#define checkError() _check_error(__FILE__, __LINE__)
#endif // !NDEBUG

// 一个可写的uniform buffer对象的封装
template <class T> struct WritableUniformBuffer {
    // 在初始化opengl后才能初始化
    WritableUniformBuffer() {
        assert(id == 0);
        glGenBuffers(1, &id);
        glBindBuffer(GL_UNIFORM_BUFFER, id);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
    }

    T *map() {
        void *ptr = glMapNamedBuffer(id, GL_WRITE_ONLY);
        assert(ptr != nullptr);
        return (T *)ptr;
    }
    void unmap() {
        bool ret = glUnmapNamedBuffer(id);
        assert(ret);
    }

    void bind(unsigned int binding_point){
        glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, id);
    }

    ~WritableUniformBuffer(){
        glDeleteBuffers(1, &id);
    }

private:
    unsigned int id = 0;
};