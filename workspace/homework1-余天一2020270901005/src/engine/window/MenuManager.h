#pragma once
#include "../utils/winapi.h"
#include <functional>
#include <iostream>
#include <assert.h>

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

    void display_pop_menu(HWND hwnd, WORD x, WORD y);

    void clear_pop_menu();

    void on_click(WORD id) { click_list[id](); }
};