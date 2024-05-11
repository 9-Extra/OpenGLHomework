#include "Display.h"

Display::Display(uint32_t width, uint32_t height) {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    const WNDCLASSEXW window_class = {
        sizeof(WNDCLASSEXW),         CS_OWNDC, WindowProc, 0,           0,   hInstance, LoadIcon(NULL, IDI_WINLOGO),
        LoadCursor(NULL, IDC_ARROW), NULL,     NULL,       L"MyWindow", NULL};

    RegisterClassExW(&window_class);
    RECT rect = {100, 100, 100 + (LONG)width, 100 + (LONG)height};
    AdjustWindowRectEx(&rect, WINDOW_STYLE, false, 0);

    hwnd = CreateWindowExW(0, L"MyWindow", MY_TITLE, WINDOW_STYLE, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                           NULL, NULL, hInstance, nullptr);

    if (hwnd == NULL) {
        assert(false);
    }
}
