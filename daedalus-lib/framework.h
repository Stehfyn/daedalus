#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <WindowsX.h>
#include <GL/GL.h>
#include <dwmapi.h>
#include <vssym32.h>
#include <shellapi.h>
#include <uxtheme.h>
#include "swcadef.h"
#include <versionhelpers.h>
#pragma comment(lib, "dwmapi.lib")
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "opengl32.lib")
static const auto W10SetWindowCompositionAttribute =
reinterpret_cast<PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE>(GetProcAddress(GetModuleHandle(L"user32.dll"), "SetWindowCompositionAttribute"));
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#define EXTERNC extern "C"
typedef struct WGL_WindowData_s { HDC hDC; } WGL_WindowData;
// Data
