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

    OpenglContext(HWND hwnd) {
        assert(hglrc == NULL);
        hdc = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
            1,                             // version number
            PFD_DRAW_TO_WINDOW |           // support window
                PFD_SUPPORT_OPENGL |       // support OpenGL
                PFD_DOUBLEBUFFER,          // double buffered
            PFD_TYPE_RGBA,                 // RGBA type
            24,                            // 24-bit color depth
            0,
            0,
            0,
            0,
            0,
            0, // color bits ignored
            0, // no alpha buffer
            0, // shift bit ignored
            0, // no accumulation buffer
            0,
            0,
            0,
            0,              // accum bits ignored
            24,             // 24-bit z-buffer
            8,              // stencil buffer
            0,              // no auxiliary buffer
            PFD_MAIN_PLANE, // main layer
            0,              // reserved
            0,
            0,
            0 // layer masks ignored
        };

        int iPixelFormat = ChoosePixelFormat(hdc, &pfd);

        // make that the pixel format of the device context
        SetPixelFormat(hdc, iPixelFormat, &pfd);

        // create a rendering context
        hglrc = wglCreateContext(hdc);
        if (hglrc == NULL) {
            exit(-1);
        }

        // make it the calling thread's current rendering context
        if (!wglMakeCurrent(hdc, hglrc)) {
            exit(-1);
        }
    }

    ~OpenglContext() {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc);
    }

private:
    HDC hdc;
    HGLRC hglrc{NULL};
};
