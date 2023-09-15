#pragma once

#include "GuidAlloctor.h"
#include "winapi.h"
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
        RECT rect = {100, 100, 100 + (LONG)width, 100 + (LONG)height};
        AdjustWindowRectEx(&rect, WINDOW_STYLE, false, 0);

        hwnd = CreateWindowExW(0, L"MyWindow", MY_TITLE, WINDOW_STYLE, 0, 0,
                               rect.right - rect.left, rect.bottom - rect.top,
                               NULL, NULL, hInstance, nullptr);

        if (hwnd == NULL) {
            assert(false);
        }
    }

    void grab_mouse(){
        while (ShowCursor(false) >= 0);
        RECT WndRect;
        GetCursorPos(&last_mouse_position);
        GetWindowRect(hwnd, &WndRect);
        ClipCursor(&WndRect);
    }

    void release_mouse(){
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

    friend class MenuBuilder;
    // 对执行此函数的线程绑定opengl上下文
};
class MenuManager {
public:
    using MenuClickCallBack = std::function<void()>;
    class MenuBuilder {
        //隐藏MenuManager的太多函数
    public:
        //唯一的功能：创建菜单项
        void set_menu(UINT flag, LPCWSTR name, std::function<void()> on_click) {
            uint32_t id = click_list.size();
            click_list.emplace_back(on_click);
            AppendMenuW(h_menu, flag, id, name);
        }
    private:
        HMENU h_menu;
        std::vector<MenuClickCallBack> &click_list;

        friend class MenuManager;
        MenuBuilder(HMENU h_menu,
                    std::vector<MenuClickCallBack> &click_list)
            : h_menu(h_menu), click_list(click_list) {}
    };
    using MenuBuildCallBack = std::function<void(MenuBuilder&)>;

    void add_item(const std::string& key, MenuBuildCallBack build_callback){
        menu_build_list.emplace(key, build_callback);
    }

    void remove_item(const std::string& key){
        menu_build_list.erase(key);
    }

private:
    HMENU h_menu;
    std::unordered_map<std::string, MenuBuildCallBack> menu_build_list;
    std::vector<MenuClickCallBack> click_list;

    friend class GlobalRuntime;
    friend LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    MenuManager() { h_menu = CreatePopupMenu(); }
    ~MenuManager() { DestroyMenu(h_menu); }

    void display_pop_menu(HWND hwnd, WORD x, WORD y) {
        on_close_pop_menu();//在下次显示菜单时才清空上次的项

        MenuBuilder builder(h_menu, click_list);
        for (auto& [_, func] : menu_build_list) {
            func(builder);
        }
        TrackPopupMenu(h_menu, 0, x, y, 0, hwnd, NULL);
    }

    void on_close_pop_menu() {
        // 清空所有目录项
        int count = GetMenuItemCount(h_menu);
        if (count == -1){
            std::cerr << "获取菜单项个数失败: " << GetLastError() << std::endl;
            exit(-1);
        }
        for (int i = 0; i < count; i++) {
            DeleteMenu(h_menu, (UINT)i, MF_BYCOMMAND);
        }
        assert(GetMenuItemCount(h_menu) == 0);
        click_list.clear();
    }

    void on_click(WORD id) { click_list[id](); }
};
