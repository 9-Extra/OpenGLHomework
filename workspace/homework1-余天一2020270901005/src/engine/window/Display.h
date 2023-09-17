#pragma once

#include "../utils/GuidAlloctor.h"
#include "../utils/winapi.h"
#include <assert.h>
#include <cstdint>
#include <functional>
#include <iostream>

#define MY_TITLE L"2020270901005 作业1"

extern LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam,
                                   LPARAM lParam);
class Display {
public:
    void show() { ShowWindow(hwnd, SW_SHOW); }
    void set_title(LPCWSTR title) { SetWindowTextW(hwnd, title); }
    HWND get_hwnd(){
        return hwnd;
    }

    Display(uint32_t width, uint32_t height);

    // void grab_mouse_tick(){
    //     if (is_mouse_grab){
    //         RECT WndRect;
    //         GetWindowRect(hwnd, &WndRect);
    //         //将鼠标移到窗口中间
    //         SetCursorPos((WndRect.left + WndRect.right) / 2, (WndRect.bottom + WndRect.top) / 2);
    //     }
    // }

    void grab_mouse(){
        while (ShowCursor(false) >= 0);
        is_mouse_grab = true;
        RECT WndRect;
        GetCursorPos(&last_mouse_position);
        GetWindowRect(hwnd, &WndRect);
        ClipCursor(&WndRect);
    }

    void release_mouse(){
        is_mouse_grab = false;
        ClipCursor(NULL);
        SetCursorPos(last_mouse_position.x, last_mouse_position.y);
        while (ShowCursor(true) < 0);
    }

    ~Display() { DestroyWindow(hwnd); }

private:
    static const DWORD WINDOW_STYLE =
        WS_CAPTION | WS_SYSMENU | WS_OVERLAPPEDWINDOW;

    HWND hwnd;
    POINT last_mouse_position;

    bool is_mouse_grab = false;

    friend class MenuBuilder;
    // 对执行此函数的线程绑定opengl上下文
};
