#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <shellapi.h>
#include <stdbool.h>

#define WM_TRAY (WM_APP + 1)
#define ID_TRAY 1
#define ID_TOGGLE 100
#define ID_FOREVER 101
#define ID_30MIN 102
#define ID_1HOUR 103
#define ID_2HOUR 104
#define ID_QUIT 105
#define TIMER_ID 1
#define IDI_APP_ICON 201

static NOTIFYICONDATAW tray;
static bool active;
static HWND window_handle;

static void set_active(bool enabled, UINT seconds) {
    KillTimer(window_handle, TIMER_ID);
    if (enabled) {
        EXECUTION_STATE result = SetThreadExecutionState(
            ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
        if (result == 0) return;
        active = true;
        if (seconds) SetTimer(window_handle, TIMER_ID, seconds * 1000u, NULL);
    } else {
        SetThreadExecutionState(ES_CONTINUOUS);
        active = false;
    }
    tray.uFlags = NIF_TIP;
    lstrcpynW(tray.szTip, active ? L"laser-ptr · Awake mode on" : L"laser-ptr · Awake mode off", ARRAYSIZE(tray.szTip));
    Shell_NotifyIconW(NIM_MODIFY, &tray);
}

static void append_item(HMENU menu, UINT id, const wchar_t *label) {
    AppendMenuW(menu, MF_STRING, id, label);
}

static void show_menu(void) {
    HMENU menu = CreatePopupMenu();
    append_item(menu, ID_TOGGLE, active ? L"Turn Off" : L"Keep Awake");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    append_item(menu, ID_FOREVER, L"Keep Awake Indefinitely");
    append_item(menu, ID_30MIN, L"Keep Awake 30 Minutes");
    append_item(menu, ID_1HOUR, L"Keep Awake 1 Hour");
    append_item(menu, ID_2HOUR, L"Keep Awake 2 Hours");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    append_item(menu, ID_QUIT, L"Quit laser-ptr");
    POINT point;
    GetCursorPos(&point);
    SetForegroundWindow(window_handle);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, point.x, point.y, 0, window_handle, NULL);
    DestroyMenu(menu);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE:
        window_handle = hwnd;
        tray.cbSize = sizeof(tray);
        tray.hWnd = hwnd;
        tray.uID = ID_TRAY;
        tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        tray.uCallbackMessage = WM_TRAY;
        tray.hIcon = LoadIconW((HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE),
                               MAKEINTRESOURCEW(IDI_APP_ICON));
        lstrcpynW(tray.szTip, L"laser-ptr · Awake mode off", ARRAYSIZE(tray.szTip));
        Shell_NotifyIconW(NIM_ADD, &tray);
        return 0;
    case WM_TRAY:
        if (lparam == WM_RBUTTONUP || lparam == WM_CONTEXTMENU) show_menu();
        else if (lparam == WM_LBUTTONUP) set_active(!active, 0);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case ID_TOGGLE: set_active(!active, 0); break;
        case ID_FOREVER: set_active(true, 0); break;
        case ID_30MIN: set_active(true, 30u * 60u); break;
        case ID_1HOUR: set_active(true, 60u * 60u); break;
        case ID_2HOUR: set_active(true, 2u * 60u * 60u); break;
        case ID_QUIT: DestroyWindow(hwnd); break;
        }
        return 0;
    case WM_TIMER:
        if (wparam == TIMER_ID) set_active(false, 0);
        return 0;
    case WM_DESTROY:
        set_active(false, 0);
        Shell_NotifyIconW(NIM_DELETE, &tray);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show) {
    (void)previous; (void)command_line; (void)show;
    const wchar_t *class_name = L"LaserPTRTrayWindow";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.lpszClassName = class_name;
    if (!RegisterClassW(&wc)) return 1;
    HWND hwnd = CreateWindowExW(0, class_name, L"laser-ptr", 0, 0, 0, 0, 0,
                                HWND_MESSAGE, NULL, instance, NULL);
    if (!hwnd) return 1;
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return (int)message.wParam;
}
