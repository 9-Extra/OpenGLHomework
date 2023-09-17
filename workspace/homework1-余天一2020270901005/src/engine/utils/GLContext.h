#pragma once

#include "winapi.h"
#include <assert.h>
#include <iostream>

class OpenglContext {
public:
    void swap() {
        if (!SwapBuffers(hdc)) {
            std::cerr << "交换前后缓冲失败\n";
            exit(-1);
        }
    }

    OpenglContext(HWND hwnd);

    ~OpenglContext();

private:
    HDC hdc;
    HGLRC hglrc{NULL};
};
