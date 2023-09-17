#include "MenuManager.h"

void MenuManager::clear_pop_menu() {
    // 清空所有目录项
    int count = GetMenuItemCount(h_menu);
    if (count == -1) {
        std::cerr << "获取菜单项个数失败: " << GetLastError() << std::endl;
        exit(-1);
    }
    for (int i = 0; i < count; i++) {
        DeleteMenu(h_menu, (UINT)i, MF_BYCOMMAND);
    }
    assert(GetMenuItemCount(h_menu) == 0);
    click_list.clear();
}
void MenuManager::display_pop_menu(HWND hwnd, WORD x, WORD y) {
    clear_pop_menu(); // 在下次显示菜单时才清空上次的项
    // 重新设置所有新的菜单项
    MenuBuilder builder(h_menu, click_list);
    for (auto &[_, func] : menu_build_list) {
        func(builder);
    }
    // 弹出菜单
    TrackPopupMenu(h_menu, 0, x, y, 0, hwnd, NULL);
}
