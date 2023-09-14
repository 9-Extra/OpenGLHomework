#pragma once

#include "winapi.h"
#include <assert.h>
#include <cstdint>
#include <iostream>

extern LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam,
                                   LPARAM lParam);

class Display {
public:
    void show() { ShowWindow(hwnd, SW_SHOW); }
    void set_title(LPCWSTR title) { SetWindowTextW(hwnd, title); }
    bool poll_events() {
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) {
                    return true;
                }
            }
            return false;
        }
    }

    void swap() {
        if (!SwapBuffers (hdc)) {
            std::cerr << "交换前后缓冲失败\n";
            exit(-1);
        }
    }

    Display(uint32_t width, uint32_t height) {
        HINSTANCE hInstance = GetModuleHandleW(NULL);

        const WNDCLASSEXW window_class = {sizeof(WNDCLASSEXW),
                                          CS_OWNDC,
                                          WindowProc,
                                          0,
                                          0,
                                          hInstance,
                                          LoadIcon(NULL, IDI_WINLOGO),
                                          LoadCursor(NULL, IDC_ARROW),
                                          NULL,
                                          NULL,
                                          L"MyWindow",
                                          NULL};

        RegisterClassExW(&window_class);
        RECT rect = {100, 100, 100 + (LONG)width, (LONG)height};
        AdjustWindowRectEx(&rect, WINDOW_STYLE, false, 0);

        hwnd = CreateWindowExW(0, L"MyWindow", L"标题", WINDOW_STYLE, 0, 0,
                               rect.right - rect.left, rect.bottom - rect.top,
                               NULL, NULL, hInstance, nullptr);

        if (hwnd == NULL) {
            assert(false);
        }
    }
    ~Display() { DestroyWindow(hwnd); }

private:
    static const DWORD WINDOW_STYLE =
        WS_CAPTION | WS_SYSMENU | WS_OVERLAPPEDWINDOW;

    HDC hdc;
    HWND hwnd;
    HGLRC hglrc{NULL};

    friend void opengl_init(void);
    friend void render_thread_func();
    // 对执行此函数的线程绑定opengl上下文
    void bind_opengl_context() {
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

    void delete_opengl_context() {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc);
    }
    
};
