// Screenshot helper for validating the Windows c11 build (usable when
// PowerShell screen capture is unavailable). Captures to PNG via GDI + GDI+.
//
// Usage:
//   screenshot.exe out.png                 capture the whole virtual screen
//   screenshot.exe out.png fg              capture the current foreground window
//   screenshot.exe out.png list            print all visible top-level windows
//   screenshot.exe out.png find:<title>    capture the window whose title == <title>
//
// For a specific window it raises it topmost and screen-captures the real
// composited pixels: PrintWindow can't reproduce externally GL-rendered content
// (ghostty's WGL SwapBuffers straight to the HWND), so a plain screen BitBlt of
// the raised window is the only reliable way to see the terminal.
//
// Build: tools/build_ss.bat (MSVC). NOTE: GDI+ headers require the min/max
// macros and IStream, so do NOT define NOMINMAX or WIN32_LEAN_AND_MEAN here.
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    ImageCodecInfo* info = (ImageCodecInfo*)malloc(size);
    if (!info) return -1;
    GetImageEncoders(num, size, info);
    int ret = -1;
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(info[i].MimeType, format) == 0) { *pClsid = info[i].Clsid; ret = (int)i; break; }
    }
    free(info);
    return ret;
}

static char g_find[256];
static HWND g_found = NULL;
static BOOL CALLBACK enumProc(HWND hwnd, LPARAM lp) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    char title[512];
    int n = GetWindowTextA(hwnd, title, sizeof(title));
    if (n <= 0) return TRUE;
    bool listing = (lp == 1);
    if (listing) {
        RECT r{}; GetWindowRect(hwnd, &r);
        printf("  hwnd=%p [%dx%d] '%s'\n", (void*)hwnd, (int)(r.right-r.left), (int)(r.bottom-r.top), title);
    } else if (g_find[0] && strcmp(title, g_find) == 0 && !g_found) {
        g_found = hwnd;
        printf("matched '%s' hwnd=%p\n", title, (void*)hwnd);
    }
    return TRUE;
}

int main(int argc, char** argv) {
    const char* out = argc > 1 ? argv[1] : "screenshot.png";
    // arg2 == "fg" -> foreground window; "list" -> print all visible windows;
    //                 "find:<substr>" -> capture window whose title contains substr.
    bool fg = (argc > 2 && strcmp(argv[2], "fg") == 0);
    bool listing = (argc > 2 && strcmp(argv[2], "list") == 0);
    const char* findArg = (argc > 2 && strncmp(argv[2], "find:", 5) == 0) ? argv[2] + 5 : NULL;
    SetProcessDPIAware();

    if (listing) { printf("visible top-level windows:\n"); EnumWindows(enumProc, 1); return 0; }

    int x, y, w, h;
    if (fg || findArg) {
        HWND hwnd = GetForegroundWindow();
        if (findArg) {
            strncpy(g_find, findArg, sizeof(g_find)-1);
            EnumWindows(enumProc, 0);
            if (!g_found) { printf("no window matching '%s'\n", findArg); return 1; }
            hwnd = g_found;
            SetForegroundWindow(hwnd);
            Sleep(400);
        }
        RECT r{};
        GetWindowRect(hwnd, &r);
        x = r.left; y = r.top; w = r.right - r.left; h = r.bottom - r.top;
        printf("target hwnd=%p rect=(%d,%d %dx%d)\n", (void*)hwnd, x, y, w, h);
    } else {
        x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }
    if (w <= 0 || h <= 0) { printf("bad rect\n"); return 1; }

    HWND target = (fg || findArg) ? (findArg ? g_found : GetForegroundWindow()) : NULL;

    // Force the target above everything and screen-capture the real composited
    // pixels. Externally GL-rendered content (ghostty's WGL SwapBuffers straight
    // to the HWND) is NOT reproduced by PrintWindow, so we must capture what DWM
    // actually shows. HWND_TOPMOST doesn't need foreground activation.
    if (target) {
        if (IsIconic(target)) ShowWindow(target, SW_RESTORE);
        SetWindowPos(target, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        BringWindowToTop(target);
        Sleep(900); // let DWM composite + ghostty present a frame
        RECT r{}; GetWindowRect(target, &r);
        x = r.left; y = r.top; w = r.right - r.left; h = r.bottom - r.top;
    }

    HDC screen = GetDC(NULL);
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, w, h);
    HGDIOBJ old = SelectObject(mem, bmp);
    BitBlt(mem, 0, 0, w, h, screen, x, y, SRCCOPY);
    SelectObject(mem, old);
    if (target) SetWindowPos(target, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    GdiplusStartupInput gsi;
    ULONG_PTR token;
    GdiplusStartup(&token, &gsi, NULL);
    {
        Bitmap b(bmp, NULL);
        CLSID clsid;
        if (GetEncoderClsid(L"image/png", &clsid) >= 0) {
            WCHAR wout[1024];
            MultiByteToWideChar(CP_UTF8, 0, out, -1, wout, 1024);
            Status s = b.Save(wout, &clsid, NULL);
            printf("save status=%d -> %s (%dx%d)\n", (int)s, out, w, h);
        } else {
            printf("no PNG encoder\n");
        }
    }
    GdiplusShutdown(token);

    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(NULL, screen);
    return 0;
}
