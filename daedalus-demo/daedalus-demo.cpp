// daedalus-demo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#ifndef GET_X_PARAM
#define GET_X_PARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_PARAM
#define GET_Y_PARAM(lp) ((int)(short)HIWORD(lp))
#endif
extern "C" {
#include "daedalus.h" 
}
#include "imgui_demo.h"
#include <functional>
#include <vector>
#include <string>

static const auto SetWindowCompositionAttribute =
reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));
// SmartProperty notifies on value change
template <typename T>
struct SmartProperty
{
public:
    T m_Value; // The value to be changed/checked

    SmartProperty(T value)
        : m_Value(value),
        m_LastValue(value),
        m_Changed(FALSE) { }

    BOOL update()
    {
        if (m_Value == m_LastValue) m_Changed = FALSE;
        else m_Changed = TRUE;
        m_LastValue = m_Value;
        return m_Changed;
    }

    BOOL has_changed() const
    {
        return m_Changed;
    }

private:
    T m_LastValue;
    BOOL m_Changed;
};

void center_window(HWND hwnd)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);

    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    int window_width = rect.right - rect.left;
    int window_height = rect.bottom - rect.top;

    int x = (screen_width - window_width) / 2;
    int y = (screen_height - window_height) / 2;

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}
static ImGuiWindowFlags overlay_flags(void)
{
    return  ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoDocking    |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav;
}
bool SetVSync(bool sync)
{
    typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALPROC)(int);
    PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = 0;

    const char* extensions = (char*)glGetString(GL_EXTENSIONS);

    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT)
        wglSwapIntervalEXT(sync);
    return true;
}
namespace ImGui{

bool CheckboxFlagsTransform(const char* label, int* flags, int bitflag, std::function<void(int*)> transform)
{
    bool toggled = ImGui::CheckboxFlags(label, flags, bitflag);
    if (toggled) {
        transform(flags);
    }
}
bool GridInput(const char* label, ImVec2 size, RECT* r)
{
    bool value_changed = false;
    float item_width = (size.x / 2.0f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (size.x / 4.0f));
    ImGui::PushItemWidth(item_width);
    value_changed |= ImGui::InputInt(("Top###" + std::string(label) + "Top").c_str(), (int*)&r->top);
    value_changed |= ImGui::InputInt(("Left###" + std::string(label) + "Left").c_str(), (int*)&r->left);
    ImGui::SameLine();
    value_changed |= ImGui::InputInt(("Right###" + std::string(label) + "Right").c_str(), (int*)&r->right);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (size.x / 4.0f));

    value_changed |= ImGui::InputInt(("Bot###" + std::string(label) + "Bot").c_str(), (int*)&r->bottom);
    ImGui::PopItemWidth();
    return value_changed;
}
}

static std::vector<RECT> g_WindowRects;
static void populate(HWND hWnd)
{

    RECT r;
    GetWindowRect(hWnd, &r);
    //GetClientRect(hWnd, &r);

    std::vector<RECT> WindowRects;
    // Only apply offset if Multi-viewports are not enabled
    ImVec2 origin = { (float)r.left, (float)r.top };
    origin.y += 30;
    //ImVec2 origin = { 0,0 };
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows) {
        if ((!(std::string(window->Name).find("Default") != std::string::npos) &&
            (!(std::string(window->Name).find("Dock") != std::string::npos)) ||
            (std::string(window->Name).find("Dear ImGui Demo") != std::string::npos))) {
            ImVec2 pos = window->Pos;
            ImVec2 size = window->Size;
            RECT rect = { origin.x + pos.x, origin.y + pos.y, origin.x + (pos.x + size.x),
                         origin.y + (pos.y + size.y) };

            if (window->Active)
                WindowRects.push_back(rect);
        }
    }
    g_WindowRects = WindowRects;
}
static LRESULT ncchittest_hook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    POINT cursor = POINT{
                    GET_X_LPARAM(lParam),
                    GET_Y_LPARAM(lParam)
    };
    for (const auto& r : g_WindowRects) {
        if (PtInRect(&r, cursor))
        {
            return HTCLIENT;
        }
    }
    return (g_WindowRects.size()) ? HTCAPTION: HTCLIENT ;
}

static void ui(ImVec4& cc, int* clearbits, void* data)
{
    win32_window_t* w32 = (win32_window_t*)data;
    HWND hwnd = win32_window_get_hwnd(w32);
    ImGui::Begin("###Backend Window Modes", 0, overlay_flags());
    ImGui::SeparatorText("Backend Window Modes");
    static int mode = 0;
    ImGui::RadioButton("Basic",      &mode, 0);
    ImGui::RadioButton("Borderless", &mode, 1);
    static int wtnca = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON | WTNCA_NOSYSMENU;
    ImGui::SeparatorText("Window Theme Nonclient Area###WTNCA");
    ImGui::CheckboxFlagsTransform("WTNCA_NODRAWCAPTION", &wtnca, (int)WTNCA_NODRAWCAPTION, 
        [hwnd](int* wtnca) { SetWindowThemeNonClientAttributes(hwnd, WTNCA_NODRAWCAPTION, (*wtnca & WTNCA_NODRAWCAPTION)); SendMessage(hwnd, WM_THEMECHANGED, 0, 0); });
    ImGui::CheckboxFlagsTransform("WTNCA_NODRAWICON", &wtnca, (int)WTNCA_NODRAWICON,
        [hwnd](int* wtnca) { SetWindowThemeNonClientAttributes(hwnd, WTNCA_NODRAWICON, (*wtnca & WTNCA_NODRAWICON)); 
                             SendMessage(hwnd, WM_THEMECHANGED, 0, 0); });
    ImGui::CheckboxFlagsTransform("WTNCA_NOSYSMENU", &wtnca, (int)WTNCA_NOSYSMENU,
        [hwnd](int* wtnca) { SetWindowThemeNonClientAttributes(hwnd, WTNCA_NOSYSMENU, (*wtnca & WTNCA_NOSYSMENU)); SendMessage(hwnd, WM_THEMECHANGED, 0, 0); });
    
    static bool custom_client_area = TRUE;
    static win32_window_overrides_t overrides = {0};
    ImGui::SeparatorText("WM_HOOKS###WM_HOOKS");
    ImGui::Checkbox("Custom Client Area", &custom_client_area);
    if (ImGui::Checkbox("Disable WM_NCACTIVATE", &overrides.disable_ncactivate)) win32_window_set_overrides(w32, overrides);

    ImGui::SeparatorText("DWM###DWM");
    static bool ncrp_enabled = TRUE;
    static bool allow_ncpaint = TRUE;
    ImGui::SeparatorText("Nonclient Rendering Policy###DWM");
    if (ImGui::Checkbox("NCRP###DWMNCRP", &ncrp_enabled))
    {
        DWMNCRENDERINGPOLICY ncrp = (ncrp_enabled) ? DWMNCRP_ENABLED : DWMNCRP_DISABLED;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
    }
    if (ImGui::Checkbox("Allow NCPaint###NCPAINT", &allow_ncpaint))
    {

        DwmSetWindowAttribute(hwnd, DWMWA_ALLOW_NCPAINT, &allow_ncpaint, sizeof(allow_ncpaint));
    }

    ImGui::SeparatorText("Extended Frame Bounds###DWM");
    static RECT extendedFrameBounds{ 0,0,0,0 };
    if(ImGui::Button("Get EFB"))
    {
        HRESULT hr = ::DwmGetWindowAttribute(hwnd,
            DWMWA_EXTENDED_FRAME_BOUNDS,
            &extendedFrameBounds,
            sizeof(extendedFrameBounds));
    }
    ImGui::Text("EFB: {%d, %d, %d, %d} (%d, %d)", extendedFrameBounds.left, extendedFrameBounds.top, extendedFrameBounds.right, extendedFrameBounds.bottom, 
        extendedFrameBounds.right - extendedFrameBounds.left, extendedFrameBounds.bottom - extendedFrameBounds.top);
    if (ImGui::GridInput("Set Etxended Frame Bounds", ImVec2(300.0f, 0.0f), &extendedFrameBounds))
    {
        HRESULT hr = ::DwmSetWindowAttribute(hwnd,
            DWMWA_EXTENDED_FRAME_BOUNDS,
            &extendedFrameBounds,
            sizeof(extendedFrameBounds));
    }
    static MARGINS m = { 0 };
    if (ImGui::Button("DwmExtendFrameIntoClientArea"))
    {
        DwmExtendFrameIntoClientArea(hwnd, &m);
    }
    ImGui::InputInt("cyTop", &m.cyTopHeight);
    ImGui::InputInt("cxLeft", &m.cxLeftWidth); ImGui::SameLine(); ImGui::InputInt("cxRight", &m.cxRightWidth);
    ImGui::InputInt("cyBottom", &m.cyBottomHeight);
    static RECT r = { 0 };
    ImGui::SeparatorText("DwmEnableBlurBehindWindow###DWM");
    if (ImGui::GridInput("DwmEnableBlurBehindWindow", ImVec2(300.0f, 0.0f), &r))
    {
        //HRGN  hRgn = CreateRectRgn(-1, -1, -1, -1);
        HRGN  hRgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
        //HRGN  hRgn = CreateRectRgn(0, 0, 0, 0);
        //HRGN  hRgn = CreateRectRgn(1, 1, 1, 1);
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur = hRgn;
        bb.fEnable = TRUE;
        DwmEnableBlurBehindWindow(hwnd, &bb);
    }
    ImGui::SeparatorText("DWM Accent State");
    static SmartProperty<INT> accent_policy{ ACCENT_ENABLE_BLURBEHIND };
    ImGui::RadioButton("DISABLED", &accent_policy.m_Value, ACCENT_DISABLED);
    ImGui::RadioButton("GRADIENT", &accent_policy.m_Value, ACCENT_ENABLE_GRADIENT);
    ImGui::RadioButton("TRANSPARENT GRADIENT", &accent_policy.m_Value, ACCENT_ENABLE_TRANSPARENTGRADIENT);
    ImGui::RadioButton("BLUR BEHIND", &accent_policy.m_Value, ACCENT_ENABLE_BLURBEHIND);
    ImGui::RadioButton("ACRYLIC BLUR BEHIND", &accent_policy.m_Value, ACCENT_ENABLE_ACRYLICBLURBEHIND);
    ImGui::RadioButton("HOST BACKDROP", &accent_policy.m_Value, ACCENT_ENABLE_HOSTBACKDROP);
    ImGui::RadioButton("INVALID STATE", &accent_policy.m_Value, ACCENT_INVALID_STATE);
    ImGui::SeparatorText("DWM Gradient");
    static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
    static SmartProperty<INT> gradient_col = { (((int)(color.w * 255)) << 24) | (((int)(color.z * 255)) << 16) | (((int)(color.y * 255)) << 8) | ((int)(color.x * 255)) };
    ImGui::ColorPicker4("##picker", (float*)&color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
    gradient_col.m_Value = (((int)(color.w * 255)) << 24) | (((int)(color.z * 255)) << 16) | (((int)(color.y * 255)) << 8) | ((int)(color.x * 255));
    ImGui::SeparatorText("DWM Accent Flags");
    static SmartProperty<INT> accent_flags{ 1 };
    ImGui::InputInt("Accent Flags", &accent_flags.m_Value, 0, 255);
    ImGui::SeparatorText("DWM Animation id");
    static SmartProperty<INT> animation_id{ 0 };
    ImGui::SliderInt("Accent Flags##se", &animation_id.m_Value, 0, 32);
    ImGui::End();

    ImGui::Begin("###OpenGL Settings", 0, overlay_flags());
    ImGui::SeparatorText("glClearColor");
    ImGui::ColorPicker4("##picker", (float*)&cc, ImGuiColorEditFlags_NoSidePreview |
                                                 ImGuiColorEditFlags_NoSmallPreview);
    
    ImGui::SeparatorText("glClearBit");
    ImGui::CheckboxFlags("GL_COLOR_BUFFER_BIT",   clearbits, GL_COLOR_BUFFER_BIT);
    ImGui::CheckboxFlags("GL_DEPTH_BUFFER_BIT",   clearbits, GL_DEPTH_BUFFER_BIT);
    ImGui::CheckboxFlags("GL_STENCIL_BUFFER_BIT", clearbits, GL_STENCIL_BUFFER_BIT);
    ImGui::CheckboxFlags("GL_ACCUM_BUFFER_BIT",   clearbits, GL_ACCUM_BUFFER_BIT);
    
    static bool VSYNC = SetVSync(TRUE);
    if (ImGui::Checkbox("VSYNC", &VSYNC)) SetVSync(VSYNC);
    if (ImGui::Button("Center Window")) center_window(hwnd);
    ImGui::End();
    accent_policy.update();
    accent_flags.update();
    gradient_col.update();
    animation_id.update();


    static bool init_accents = false; //to apply default initialization
    if (accent_policy.has_changed() || accent_flags.has_changed()
        || gradient_col.has_changed() || animation_id.has_changed()
        || init_accents)
    {
        if (init_accents) init_accents = false;

        ACCENT_POLICY policy = {
        ACCENT_STATE(accent_policy.m_Value),
        accent_flags.m_Value,
        gradient_col.m_Value,
        animation_id.m_Value
        };

        const WINDOWCOMPOSITIONATTRIBDATA data = {
            WCA_ACCENT_POLICY,
            &policy,
            sizeof(policy)
        };

        SetWindowCompositionAttribute(hwnd, &data);
    }


    ImGui::Begin("ImGui Window Bg", 0, overlay_flags());
    ImGui::SeparatorText("ImGuiCol_WindowBgAlpha");
    static SmartProperty<float> bg_alpha{ 1 };
    ImGui::SliderFloat("BgAlpha", &bg_alpha.m_Value, 1, 0);
    if (bg_alpha.update()) {
        ImVec4 window_bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        window_bg.w = bg_alpha.m_Value;
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = window_bg;
    }
    ImGui::End();
    // Demo Overlay
    {
        ImGuiIO& io = ImGui::GetIO();
        static float f = 0.0f;
        static int counter = 0;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

        if (ImGui::Begin("Example: Simple overlay", 0, window_flags))
        {
            if (ImGui::IsMousePosValid())
                ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
            else
                ImGui::Text("Mouse Position: <invalid>");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        }
        ImGui::End();
    }

    if (custom_client_area)
        populate(hwnd);
    else
        g_WindowRects = {};

}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main()
{
    win32_window_t* w32 = win32_window_allocate();
    win32_window_create(w32, 600, 1000);

    init_imgui(win32_window_get_hwnd(w32));
    win32_window_set_wndproc_hook(w32, ImGui_ImplWin32_WndProcHandler);
    win32_window_set_nchittest_hook(w32, ncchittest_hook);
    MSG msg;
    for (;;) {
        if (win32_window_pump_message_loop(w32, &msg)) {
            if (msg.message == WM_QUIT) break;
        }
        
        render_imgui(ui, win32_window_get_devicecontext(w32), w32);
    }

    //win32_window_sync_visual(w32);
    //win32_window_run_message_loop(w32);
    //while (true) { (void)win32_window_pump_message_loop(w32); }
}
