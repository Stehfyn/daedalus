// daedalus-lib.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include "daedalus_win32.h"

struct win32_window_s
{
    
    HWND                       hWnd;
    win32_window_data_t        data;
    win32_window_attributes_t  attrs;
    win32_window_attributes_t  dirty;
    HGLRC                      hRC;
    WGL_WindowData             wgldata;
    LRESULT(*hook)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT(*nchittest_hook)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    win32_window_s* pThis;
    win32_window_overrides_t overrides;
};
#include "daedalus_win32_helpers.h"

enum class style : DWORD
{
    windowed = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
    aero_borderless = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
    basic_borderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX
};

static HGLRC g_hRC;
//================================================================================
extern "C"
LRESULT WINAPI
win32_wndproc(
    HWND   hWnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        LPCREATESTRUCTW pCreateStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        if (pCreateStruct)
        {
            ::SetWindowLongPtrW(hWnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        SetWindowThemeNonClientAttributes(hWnd, WTNCA_NODRAWCAPTION, WTNCA_NODRAWCAPTION);
        SetWindowThemeNonClientAttributes(hWnd, WTNCA_NODRAWICON, WTNCA_NODRAWICON);
        SetWindowThemeNonClientAttributes(hWnd, WTNCA_NOSYSMENU, WTNCA_NOSYSMENU);
        
        //DWORD dwFlags = (STAP_ALLOW_NONCLIENT);
        //SetThemeAppProperties(dwFlags);
        //SendMessage(hWnd, WM_THEMECHANGED, 0, 0 );
    }
    else if (win32_window_t* w32 =
        reinterpret_cast<win32_window_t*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA)))
    {
        if (w32->hook)
        {
            if (w32->hook(hWnd, msg, wParam, lParam)) return TRUE;
        }

        switch (msg) {
        case WM_DESTROY:    ::PostQuitMessage(0); return 0;
        //case WM_ERASEBKGND:
        //    return 1;
        case WM_CREATE: {
            
            break;
        }
        //case WM_NCCALCSIZE: return calc_nonclient_size(hWnd, msg, wParam, lParam);
        case WM_NCCALCSIZE: {
            return handle_nccalcsize(hWnd, wParam, lParam);
           
        }
        case WM_NCHITTEST: {
            LRESULT result = calc_nonclient_hits(hWnd, msg, wParam, lParam);
            LRESULT hook_result  = HTCLIENT;
            if (w32->nchittest_hook) hook_result = w32->nchittest_hook(hWnd, msg, wParam, lParam);

            if (result == HTCLIENT) return hook_result;
            return result;
        }
        case WM_NCACTIVATE: {
            //return 1;
            //if (w32->overrides.disable_ncactivate) return 1;
            break;
        } 
        case WM_ACTIVATE:
        {
            extend_nonclient_frame(hWnd, FALSE);
            set_composition(hWnd, TRUE);
            sync_frame_change(hWnd);

        }break;
        case WM_GETMINMAXINFO:
        {
            DefWindowProc(hWnd, msg, wParam, lParam);
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = { 0 };
            mi.cbSize = sizeof mi;
            GetMonitorInfoW(mon, &mi);
            RECT r = mi.rcMonitor;
            AdjustMaximizedRect(hWnd, &mi, &r, FALSE);
            mmi->ptMaxPosition.x = r.left - 2;
            mmi->ptMaxPosition.y = r.top;
            mmi->ptMaxSize.x = RECTWIDTH(r) + 4;
            mmi->ptMaxSize.y = RECTHEIGHT(r);
            printf("WM_GETMINMAXINFO\n");
            return 0;
        }
        case WM_WINDOWPOSCHANGING:
        {

            //printf("WM_WINDOWPOSCHANGING\n");
            break;
        }
        case WM_SIZE:
        {
            //printf("WM_SIZE\n");
            break;
        }
        case 0xAE:
        case 0xAF:
        {
            /* These undocumented messages are sent to draw themed window borders.
               Block them to prevent drawing borders over the client area. */
            return 0;
        }
        }
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

extern "C"
win32_window_t*
win32_window_allocate()
{
    win32_window_t** w32_window;

    w32_window  = reinterpret_cast<win32_window_t**>(malloc(sizeof(win32_window_t*)));
    assert(w32_window); SecureZeroMemory(w32_window, sizeof(win32_window_t*));
    *w32_window = reinterpret_cast<win32_window_t*>(malloc(sizeof(win32_window_t)));
    assert(*w32_window); SecureZeroMemory(*w32_window, sizeof(win32_window_t));

    (*w32_window)->pThis = *w32_window;

    return *w32_window;
}


EXTERNC VOID
win32_window_create(
    win32_window_t* w32Window,
    INT width,
    INT height)
{
    w32Window->attrs = w32Window->dirty;
    WNDCLASSEXW wcex;
    {
        SecureZeroMemory(&wcex, sizeof(wcex));

        wcex.cbSize = sizeof(wcex);
        wcex.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        wcex.lpfnWndProc = (WNDPROC)win32_wndproc;
        wcex.hInstance = nullptr;
        wcex.lpszClassName = L"daedulus";
        wcex.hbrBackground = (HBRUSH)(::GetStockObject(BLACK_BRUSH)); //Transparency :)
        wcex.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        CONST ATOM _ = ::RegisterClassExW(&wcex);
        assert(_);
    }
    //WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE,
    //WS_POPUP | WS_THICKFRAME |  WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE, //borderless
    {
        w32Window->hWnd = CreateWindowExW(
            WS_EX_APPWINDOW | WS_EX_STATICEDGE | WS_EX_CLIENTEDGE | WS_EX_NOREDIRECTIONBITMAP,
            wcex.lpszClassName,
            L"daedulus-demo",
            WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr, nullptr, nullptr, w32Window->pThis
        );

        assert(w32Window->hWnd);
        assert(GetClassNameW(w32Window->hWnd, w32Window->data.wcClassName, 256));
    }

    //sync_frame_change(w32Window->hWnd);
    w32Window->overrides.disable_ncactivate = FALSE;
    CreateDeviceWGL(w32Window->hWnd, &w32Window->wgldata, &g_hRC);
    w32Window->hRC = g_hRC;
    wglMakeCurrent(w32Window->wgldata.hDC, w32Window->hRC);

}

EXTERNC VOID
win32_window_sync_visual(
    win32_window_t* w32Window)
{
    extend_nonclient_frame(w32Window->hWnd, FALSE);
    set_composition(w32Window->hWnd, TRUE);
    sync_frame_change(w32Window->hWnd);
}

EXTERNC BOOL
win32_window_pump_message_loop(
    win32_window_t* w32Window, MSG* msg)
{
    BOOL found = ::PeekMessageW(msg, nullptr, 0, 0, PM_REMOVE);
    if (found) {
        ::TranslateMessage(msg);
        ::DispatchMessageW(msg);
    }
    return found;
}

EXTERNC VOID 
win32_window_run_message_loop(
    win32_window_t* w32Window)
{
    for (;;) {
        MSG msg = { 0 };
        BOOL result = GetMessageW(&msg, 0, 0, 0);
        if (result > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                return;
            
            if (IsMaximized(w32Window->hWnd))
            {
                Sleep(100);
                continue;
            }
                draw(w32Window->hWnd, w32Window);
            continue;
        }
        //break;
    }
}

EXTERNC HWND win32_window_get_hwnd(win32_window_t* w32Window)
{
    return w32Window->hWnd;
}

EXTERNC HGLRC win32_window_get_glcontext(win32_window_t* w32Window)
{
    return w32Window->hRC;
}

EXTERNC HDC win32_window_get_devicecontext(win32_window_t* w32Window)
{
    return w32Window->wgldata.hDC;
}

EXTERNC VOID win32_window_set_wndproc_hook(win32_window_t* w32Window, LRESULT(*hook)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam))
{
    w32Window->hook = hook;
}

EXTERNC VOID win32_window_set_nchittest_hook(win32_window_t* w32Window, LRESULT(*hook)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam))
{
    w32Window->nchittest_hook = hook;
}

EXTERNC VOID win32_window_set_overrides(win32_window_t* w32Window, win32_window_overrides_t overrides)
{
    w32Window->overrides = overrides;
}

