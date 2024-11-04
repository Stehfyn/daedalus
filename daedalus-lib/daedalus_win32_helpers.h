#pragma once

#include "framework.h"
#ifndef GET_X_PARAM
#define GET_X_PARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_PARAM
#define GET_Y_PARAM(lp) ((int)(short)HIWORD(lp))
#endif

#ifndef RECTWIDTH
#define RECTWIDTH(r)\
    (r.right - r.left)
#endif
#ifndef RECTHEIGHT
#define RECTHEIGHT(r)\
    (r.bottom - r.top)
#endif
static INT
win32_dpi_scale(
    int value,
    UINT dpi
) {
    return (int)((float)value * dpi / 96);
}

static VOID
extend_nonclient_frame(
    HWND hWnd,
    BOOL enabled
)
{
    static const MARGINS margins[2] = { {0,0,0,0}, {-1,-1,-1,-1} };
    ::DwmExtendFrameIntoClientArea(hWnd, &margins[enabled]);
}

static VOID
set_composition(
    HWND hWnd,
    BOOL enabled)
{

    //HRGN  hRgn = CreateRectRgn(-1, -1, -1, -1);
    HRGN  hRgn = CreateRectRgn(0, 0, -1, -1);
    //HRGN  hRgn = CreateRectRgn(0, 0, 0, 0);
    //HRGN  hRgn = CreateRectRgn(1, 1, 1, 1);
    DWM_BLURBEHIND bb = { 0 };
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.hRgnBlur = hRgn;
    bb.fEnable = enabled;
    DwmEnableBlurBehindWindow(hWnd, &bb);
}

static VOID
sync_frame_change(
    HWND hWnd
)
{
    ::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE       |
                                              SWP_NOSIZE       |
                                              SWP_FRAMECHANGED |
                                              SWP_SHOWWINDOW);
}


static RECT
win32_titlebar_rect(
    HWND handle
) {
    SIZE title_bar_size = { 0 };
    const int top_and_bottom_borders = 2;
    HTHEME theme = OpenThemeData(handle, L"WINDOW");
    UINT dpi = GetDpiForWindow(handle);
    GetThemePartSize(theme, NULL, WP_CAPTION, CS_ACTIVE, NULL, TS_TRUE, &title_bar_size);
    CloseThemeData(theme);

    int height = win32_dpi_scale(title_bar_size.cy, dpi) + top_and_bottom_borders;

    RECT rect;
    GetClientRect(handle, &rect);
    rect.bottom = rect.top + height;
    return rect;
}

static BOOL
is_maximized(HWND hwnd)
{
    WINDOWPLACEMENT placement{};
    SecureZeroMemory(&placement, sizeof(WINDOWPLACEMENT));

    if (!::GetWindowPlacement(hwnd, &placement)) return false;
    return placement.showCmd == SW_MAXIMIZE;
}

static VOID
adjust_maximized_client_rect(
    HWND  hWnd,
    RECT* rClient)
{
    if (!is_maximized(hWnd)) return;

    HMONITOR monitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
    if (!monitor) return;

    MONITORINFO monitor_info;
    SecureZeroMemory(&monitor_info, sizeof(MONITORINFO));

    monitor_info.cbSize = sizeof(monitor_info);
    if (!::GetMonitorInfoW(monitor, &monitor_info)) return;

    // when maximized, make the client area fill just the monitor (without task bar) rect,
    // not the whole window rect which extends beyond the monitor.
    *rClient = monitor_info.rcWork;
}

static LRESULT
calc_nonclient_size(
    HWND   hWnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT default_result = ::DefWindowProcW(hWnd, msg, wParam, lParam);
    if (!wParam) return default_result;

    UINT dpi = GetDpiForWindow(hWnd);
    int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
    int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

    NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
    RECT* requested_client_rect = params->rgrc;

    requested_client_rect->left += frame_x + padding;
    requested_client_rect->right -= frame_x + padding;
    requested_client_rect->bottom -= frame_y + padding;
    requested_client_rect->bottom--;

    if (is_maximized(hWnd)) {
        requested_client_rect->top += padding + ::GetSystemMetrics(SM_CYSMSIZE);
    }
    else {
        requested_client_rect->top-=2;
    }

    return 0;
}

LRESULT hit_test(HWND hWnd, POINT cursor)
{
    const POINT border{
        ::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
        ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
    };
    const UINT b_width = ::GetSystemMetrics(SM_CXSIZE);
    const UINT b_height = ::GetSystemMetrics(SM_CYCAPTION);

    RECT window{};
    if (!::GetWindowRect(hWnd, &window)) return HTNOWHERE;
    const RECT close_{
       window.right - b_width,
       window.top,
       window.right,
       window.top + b_height
    };

    const RECT max_{
       window.right - (b_width * 2),
       window.top,
       window.right,
       window.top + b_height
    };

    const RECT min_{
       window.right - (b_width * 3),
       window.top,
       window.right,
       window.top + b_height
    };

    if (PtInRect(&close_, cursor))
        return HTCLOSE;

    else if (PtInRect(&max_, cursor))
        return HTMAXBUTTON;

    else if (PtInRect(&min_, cursor))
        return HTMINBUTTON;

    CONST UINT drag = HTCAPTION;

    enum region_mask {
        client = 0b0000,
        left = 0b0001,
        right = 0b0010,
        top = 0b0100,
        bottom = 0b1000,
    };

    const auto result =
        left * (cursor.x < (window.left + border.x)) |
        right * (cursor.x >= (window.right - border.x)) |
        top * (cursor.y < (window.top + border.y)) |
        bottom * (cursor.y >= (window.bottom - border.y));

    switch (result) {
    case left: return HTLEFT;
    case right: return HTRIGHT;
    case top: return HTTOP;
    case bottom: return HTBOTTOM;
    case top | left: return HTTOPLEFT;
    case top | right: return HTTOPRIGHT;
    case bottom | left: return  HTBOTTOMLEFT;
    case bottom | right: return HTBOTTOMRIGHT;
    case client: return drag;
    default: return HTNOWHERE;
    }
}

static
LRESULT
calc_nonclient_hits(
    HWND   hWnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL    dwm_handled = FALSE;
    LRESULT dwm_result = 0;
    LRESULT default_result = 0;

    dwm_handled = ::DwmDefWindowProc(hWnd, msg, wParam, lParam, &dwm_result);
    if (dwm_handled) {
        RECT rr;
        rr = win32_titlebar_rect(hWnd);
        InvalidateRect(hWnd, &rr, TRUE);
        return dwm_result;
    }

    POINT cursor = POINT{
        GET_X_LPARAM(lParam),
        GET_Y_LPARAM(lParam)
    };
    LRESULT rrr = hit_test(hWnd, cursor);
    switch (rrr)
    {
    case HTCLOSE:
    case HTMAXBUTTON:
    case HTMINBUTTON: {

        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    default: break;
    }
    default_result = ::DefWindowProcW(hWnd, msg, wParam, lParam);
    return default_result;
    return rrr;
    //return (default_result == HTCLIENT) ? HTCAPTION : default_result;
}


static bool has_autohide_appbar(UINT edge, RECT mon)
{
    if (IsWindows8Point1OrGreater()) {
        APPBARDATA abd = { 0 };
        abd.cbSize = sizeof abd;
        abd.uEdge = edge;
        abd.rc = mon;

        return SHAppBarMessage(ABM_GETAUTOHIDEBAREX,&abd);
    }

    /* Before Windows 8.1, it was not possible to specify a monitor when
       checking for hidden appbars, so check only on the primary monitor */
    if (mon.left != 0 || mon.top != 0)
        return false;
    APPBARDATA abd = { 0 };
    abd.cbSize = sizeof abd;
    abd.uEdge = edge;
    return SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
}

static void AdjustMaximizedRect(HWND hwnd, const MONITORINFO* mi, RECT* r, BOOL use_caption)
{
    UINT dpi = GetDpiForWindow(hwnd);
    int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
    int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    int caption = GetSystemMetricsForDpi(SM_CYCAPTION, dpi);
    bool taskbar_on_x = !((mi->rcMonitor.left == mi->rcWork.left) && (mi->rcMonitor.right == mi->rcWork.right));
    bool taskbar_on_y = !((mi->rcMonitor.top == mi->rcWork.top) && (mi->rcMonitor.bottom == mi->rcWork.bottom));
    if (taskbar_on_x)
    {
        if (mi->rcMonitor.left  != mi->rcWork.left)    r->left = mi->rcWork.left;
        if (mi->rcMonitor.right != mi->rcWork.right)   r->right  = mi->rcWork.right;
    }
    if (taskbar_on_y)
    {
        if (mi->rcMonitor.top    != mi->rcWork.top)    r->top = mi->rcWork.top;
        if (mi->rcMonitor.bottom != mi->rcWork.bottom) r->bottom = mi->rcWork.bottom;
    }
    if (use_caption) {
        r->top += caption;
       // r->top += padding;
        r->top += padding;
    }
    r->top += frame_y ;
    r->top -= 3;
}

static LRESULT handle_nccalcsize(HWND window, WPARAM wparam,
    LPARAM lparam)
{
    union {
        LPARAM lparam;
        RECT* rect;
    } params{0};
    params.lparam = lparam;

    /* DefWindowProc must be called in both the maximized and non-maximized
       cases, otherwise tile/cascade windows won't work */
    RECT nonclient = *params.rect;
    DefWindowProcW(window, WM_NCCALCSIZE, wparam, params.lparam);
    RECT client = *params.rect;

    if (IsMaximized(window)) {
        WINDOWINFO wi = { 0 };
        wi.cbSize = sizeof wi;
        GetWindowInfo(window, &wi);
        UINT dpi = GetDpiForWindow(window);
        int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
        int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
        int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        //RECT r;
        //SystemParametersInfo(SPI_GETWORKAREA, NULL, &r, 0);
        /* Maximized windows always have a non-client border that hangs over
           the edge of the screen, so the size proposed by WM_NCCALCSIZE is
           fine. Just adjust the top border to remove the window title. */
        
        printf("WM_NCCHITTEST MAXIMIZED\n");
        HMONITOR mon = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { 0 };
        mi.cbSize = sizeof mi;
        GetMonitorInfoW(mon, &mi);
        RECT rr = RECT{
            client.left,
            nonclient.top + (LONG)wi.cyWindowBorders,
            client.right,
            client.bottom
        };

        {
            AdjustMaximizedRect(window, &mi, &rr, TRUE);
        }

        *params.rect = rr;


        /* If the client rectangle is the same as the monitor's rectangle,
           the shell assumes that the window has gone fullscreen, so it removes
           the topmost attribute from any auto-hide appbars, making them
           inaccessible. To avoid this, reduce the size of the client area by
           one pixel on a certain edge. The edge is chosen based on which side
           of the monitor is likely to contain an auto-hide appbar, so the
           missing client area is covered by it. */
        if (EqualRect(params.rect, &mi.rcMonitor)) {
            if (has_autohide_appbar(ABE_BOTTOM, mi.rcMonitor))
                params.rect->bottom--;
            else if (has_autohide_appbar(ABE_LEFT, mi.rcMonitor))
                params.rect->left++;
            else if (has_autohide_appbar(ABE_TOP, mi.rcMonitor))
                params.rect->top++;
            else if (has_autohide_appbar(ABE_RIGHT, mi.rcMonitor))
                params.rect->right--;
        }
    }
    else {
        /* For the non-maximized case, set the output RECT to what it was
           before WM_NCCALCSIZE modified it. This will make the client size the
           same as the non-client size. */
        *params.rect = client;
    }
    return 0;
}


static
LRESULT
calc_nonclient_hits2(
    HWND   hWnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    // Let the default procedure handle resizing areas
    LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
    switch (hit) {
    case HTNOWHERE:
    case HTRIGHT:
    case HTLEFT:
    case HTTOPLEFT:
    case HTTOP:
    case HTTOPRIGHT:
    case HTBOTTOMRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT: {
        return hit;
    }
    }
    //// Check if hover button is on maximize to support SnapLayout on Windows 11
    //if (title_bar_hovered_button == CustomTitleBarHoveredButton_Maximize) {
    //    return HTMAXBUTTON;
    //}

    // Looks like adjustment happening in NCCALCSIZE is messing with the detection
    // of the top hit area so manually fixing that.
    UINT dpi = GetDpiForWindow(hWnd);
    int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    POINT cursor_point = { 0 };
    cursor_point.x = GET_X_PARAM(lParam);
    cursor_point.y = GET_Y_PARAM(lParam);
    ScreenToClient(hWnd, &cursor_point);
    //if (cursor_point.y > 0 && cursor_point.y < frame_y + padding) {
    //    return HTTOP;
    //}

    // Since we are drawing our own caption, this needs to be a custom test
    if (cursor_point.y < win32_titlebar_rect(hWnd).bottom) {
        return HTCAPTION;
    }

    return HTCLIENT;
}

static LRESULT handle_nchittest(HWND window, int x, int y)
{
    if (IsMaximized(window))
        return HTCLIENT;

    POINT mouse = { x, y };
    ScreenToClient(window, &mouse);
    RECT client;
    GetWindowRect(window, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    /* The horizontal frame should be the same size as the vertical frame,
       since the NONCLIENTMETRICS structure does not distinguish between them */
    int frame_size = GetSystemMetrics(SM_CXFRAME) +
        GetSystemMetrics(SM_CXPADDEDBORDER);
    /* The diagonal size handles are wider than the frame */
    int diagonal_width = frame_size * 2 + GetSystemMetrics(SM_CXBORDER);

    if (mouse.y < frame_size) {
        if (mouse.x < diagonal_width)
            return HTTOPLEFT;
        if (mouse.x >= width - diagonal_width)
            return HTTOPRIGHT;
        return HTTOP;
    }

    if (mouse.y >= height - frame_size) {
        if (mouse.x < diagonal_width)
            return HTBOTTOMLEFT;
        if (mouse.x >= width - diagonal_width)
            return HTBOTTOMRIGHT;
        return HTBOTTOM;
    }

    if (mouse.x < frame_size)
        return HTLEFT;
    if (mouse.x >= width - frame_size)
        return HTRIGHT;
    return HTCLIENT;
}


bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data, HGLRC* hRC)
{
    HDC hDc = GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,                                // Version Number
      PFD_DRAW_TO_WINDOW |              // Format Must Support Window
      PFD_SUPPORT_OPENGL |              // Format Must Support OpenGL
      PFD_SUPPORT_COMPOSITION |         // Format Must Support Composition
      PFD_DOUBLEBUFFER,                 // Must Support Double Buffering
      PFD_TYPE_RGBA,                    // Request An RGBA Format
      32,                               // Select Our Color Depth
      0, 0, 0, 0, 0, 0,                 // Color Bits Ignored
      8,                                // An Alpha Buffer
      0,                                // Shift Bit Ignored
      0,                                // No Accumulation Buffer
      0, 0, 0, 0,                       // Accumulation Bits Ignored
      24,                               // 16Bit Z-Buffer (Depth Buffer)
      8,                                // Some Stencil Buffer
      0,                                // No Auxiliary Buffer
      PFD_MAIN_PLANE,                   // Main Drawing Layer
      0,                                // Reserved
      0, 0, 0                           // Layer Masks Ignored
    };

    const int pf = ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ReleaseDC(hWnd, hDc);
    //RECT r;
    //GetWindowRect(hWnd, &r);
    //g_hrgn = CreateRectRgn(0, 0, r.right - r.left, r.bottom - r.top);
    data->hDC = GetDC(hWnd);
    //data->hDC = GetDCEx(hWnd, g_hrgn, DCX_WINDOW);
    if (hRC)
        *hRC = wglCreateContext(data->hDC);
    return true;
}

static void draw(HWND hWnd, win32_window_t* w32Window)
{
    //wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    RECT r;
    if (GetWindowRect(hWnd, &r))
    {
        //glViewport(0, 0, r.right - r.left - 1, r.bottom - r.top - 1);
        glViewport(0, 0, r.right - r.left - 1, r.bottom - r.top - 1);
        glDisable(GL_BLEND);
        //glClearColor(0.5, 0.5, 0.5, 0.5);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor3f(.1f, .1f, .1f);

        // Draw a rectangle
        //glBegin(GL_QUADS);
        //glVertex2f(-1.0f, .5f); // Bottom-left corner
        //glVertex2f(1.0f, .5f); // Bottom-right corner
        //glVertex2f(1.0f, 1.0f); // Top-right corner
        //glVertex2f(-1.0f, 1.0f); // Top-left corner
        //glEnd();

        glFlush();
        SwapBuffers(w32Window->wgldata.hDC);
    }
}