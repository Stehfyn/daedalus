#pragma once
typedef enum win32_window_attributes_s
{
    WIN32_WINDOW_ATTRS_COMPOSITED = (1 << 0),
    WIN32_WINDOW_ATTRS_SHADOWED = (1 << 1),
    WIN32_WINDOW_ATTRS_BORDERLESS = (1 << 2),
    WIN32_WINDOW_ATTRS_LAST = (1 << 3),

}win32_window_attributes_t;
typedef struct win32_window_overrides_s
{
    bool disable_ncactivate;
}win32_window_overrides_t;

typedef struct win32_window_data_s
{
    WCHAR  wcClassName[256];

}win32_window_data_t;

typedef struct win32_window_s win32_window_t;

extern "C"
win32_window_t*
win32_window_allocate();

extern "C"
VOID
win32_window_create(
    win32_window_t* w32Window,
    INT width,
    INT height);

extern "C"
VOID
win32_window_sync_visual(
    win32_window_t* w32Window);

extern "C"
BOOL
win32_window_pump_message_loop(
    win32_window_t* w32Window,
    MSG* msg);

extern "C"
VOID
win32_window_run_message_loop(
    win32_window_t* w32Window);

EXTERNC HWND
win32_window_get_hwnd(
    win32_window_t* w32Window);

EXTERNC HGLRC
win32_window_get_glcontext(
    win32_window_t* w32Window);

EXTERNC HDC
win32_window_get_devicecontext(
    win32_window_t* w32Window);

EXTERNC VOID
win32_window_set_wndproc_hook(
    win32_window_t* w32Window,
    LRESULT(*hook)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
);

EXTERNC VOID
win32_window_set_nchittest_hook(
    win32_window_t* w32Window,
    LRESULT(*hook)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
);

EXTERNC VOID
win32_window_set_overrides(
    win32_window_t* w32Window,
    win32_window_overrides_t overrides
);