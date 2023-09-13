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
        if (!wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE)) {
            std::cerr << "交换前后缓冲失败\n";
            exit(-1);
        }
    }

    Display(uint32_t width, uint32_t height) {
        HINSTANCE hInstance = GetModuleHandleW(NULL);

        const WNDCLASSEXW window_class = {.cbSize = sizeof(WNDCLASSEXW),
                                          .style = CS_OWNDC,
                                          .lpfnWndProc = WindowProc,
                                          .cbClsExtra = 0,
                                          .cbWndExtra = 0,
                                          .hInstance = hInstance,
                                          .hIcon = NULL,
                                          .hCursor =
                                              LoadCursor(NULL, IDC_ARROW),
                                          .hbrBackground = NULL,
                                          .lpszMenuName = NULL,
                                          .lpszClassName = L"MyWindow",
                                          .hIconSm = NULL};

        RegisterClassExW(&window_class);
        RECT rect = {.left = 100,
                     .top = 100,
                     .right = 100 + (LONG)width,
                     .bottom = 100 + (LONG)height};
        AdjustWindowRectEx(&rect, WINDOW_STYLE, false, 0);

        hwnd = CreateWindowExW(0, L"MyWindow", L"标题", WINDOW_STYLE, 0, 0,
                               rect.right - rect.left, rect.bottom - rect.top,
                               NULL, NULL, hInstance, nullptr);

        if (hwnd == NULL) {
            assert(false);
        }

        bind_opengl_context();
    }
    ~Display() {
        wglMakeCurrent(NULL, NULL);

        // delete the rendering context
        wglDeleteContext(hglrc);
    }

private:
    static const DWORD WINDOW_STYLE =
        WS_CAPTION | WS_SYSMENU | WS_OVERLAPPEDWINDOW;

    HDC hdc;
    HWND hwnd;
    HGLRC hglrc;

    friend void opengl_init(void);
    void bind_opengl_context() {
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
            32,             // 32-bit z-buffer
            0,              // no stencil buffer
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
};
