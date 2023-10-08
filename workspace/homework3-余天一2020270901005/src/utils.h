#pragma once

#include <GL/glew.h>
#include <GL/glut.h>

#include <string>
#include <iostream>

inline void _check_error(const std::string &file, size_t line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "GL error 0x" << error << ": " << gluErrorString(error) << std::endl;
        std::cerr << "At: " << file << ':' << line << std::endl;
    }
}

#define checkError() _check_error(__FILE__, __LINE__)