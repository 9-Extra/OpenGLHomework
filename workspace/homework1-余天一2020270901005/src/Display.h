#pragma once

#include "winapi.h"
#include <assert.h>
#include <cstdint>
#include <iostream>
#include "GuidAlloctor.h"
#include <functional>

extern LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam,
                                   LPARAM lParam);

using MenuCallBack = std::function<void(uint32_t)>;
class Display {
public:
    void show() { ShowWindow(hwnd, SW_SHOW); }
    void set_title(LPCWSTR title) { SetWindowTextW(hwnd, title); }

    uint32_t append_menu_item(UINT flag, LPCWSTR name, MenuCallBack callback){
        uint32_t id = menu_item_id_alloctor.alloc_id();
        AppendMenuW(popup_menu, flag, id, name);
        menu_callback_list.emplace(id, callback);
        return id;
    }
    void modify_menu_item(uint32_t id, UINT flag, LPCWSTR name){
        ModifyMenuW(popup_menu, id, MF_BYCOMMAND | flag, id, name);
    }

    void delete_menu_item(uint32_t id){
        DeleteMenu(popup_menu, id, MF_BYCOMMAND);
        menu_callback_list.erase(id);
        menu_item_id_alloctor.free_id(id);
    }

    int get_menu_item_count(){
        return GetMenuItemCount(popup_menu);
    }

    void display_pop_menu(WPARAM lParam){
        TrackPopupMenu(popup_menu, 0, LOWORD(lParam),HIWORD(lParam), 0, hwnd, NULL);
    }
    void on_menu_click(uint32_t id){
        menu_callback_list.at(id)(id);
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
        RECT rect = {100, 100, 100 + (LONG)width, 100 +(LONG)height};
        AdjustWindowRectEx(&rect, WINDOW_STYLE, false, 0);

        hwnd = CreateWindowExW(0, L"MyWindow", L"标题", WINDOW_STYLE, 0, 0,
                               rect.right - rect.left, rect.bottom - rect.top,
                               NULL, NULL, hInstance, nullptr);

        if (hwnd == NULL) {
            assert(false);
        }

        popup_menu = CreatePopupMenu();
    }
    ~Display() {
        DestroyMenu(popup_menu);
        DestroyWindow(hwnd); 
    }

private:
    static const DWORD WINDOW_STYLE =
        WS_CAPTION | WS_SYSMENU | WS_OVERLAPPEDWINDOW;

    HDC hdc;
    HWND hwnd;
    HMENU popup_menu;
    GuidAlloctor menu_item_id_alloctor;
    std::unordered_map<uint32_t, MenuCallBack> menu_callback_list;
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
