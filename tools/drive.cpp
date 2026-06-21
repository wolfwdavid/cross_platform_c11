// Input/window driver for validating the Windows c11 build: focuses the c11
// window and synthesizes keyboard input or resizes it, so resize + typing in
// the terminal pane can be exercised without a human at the keyboard.
//
// Usage (always targets the top-level window titled "c11"):
//   drive.exe type "<text>"    focus c11 and type <text> via SendInput
//   drive.exe enter            focus c11 and press Enter
//   drive.exe maximize         maximize the c11 window
//   drive.exe restore          un-maximize / restore
//   drive.exe resize <w> <h>   resize the c11 window to w x h (logical px)
#include <windows.h>
#include <cstdio>
#include <cstring>

static HWND g_found = NULL;
static BOOL CALLBACK enumProc(HWND hwnd, LPARAM) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    char title[512];
    if (GetWindowTextA(hwnd, title, sizeof(title)) <= 0) return TRUE;
    if (strcmp(title, "c11") == 0 && !g_found) g_found = hwnd;
    return TRUE;
}

static HWND findC11() {
    g_found = NULL;
    EnumWindows(enumProc, 0);
    return g_found;
}

// Force the window to the foreground, bypassing the foreground lock by briefly
// attaching to the current foreground thread's input queue.
static void forceForeground(HWND hwnd) {
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    HWND fg = GetForegroundWindow();
    DWORD fgTid = GetWindowThreadProcessId(fg, NULL);
    DWORD myTid = GetCurrentThreadId();
    AttachThreadInput(myTid, fgTid, TRUE);
    BringWindowToTop(hwnd);
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    AttachThreadInput(myTid, fgTid, FALSE);
    Sleep(300);
}

static void typeUnicode(const char* utf8) {
    wchar_t wbuf[1024];
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, 1024);
    for (int i = 0; i < n - 1; ++i) { // n-1: skip NUL
        INPUT in[2] = {};
        in[0].type = INPUT_KEYBOARD;
        in[0].ki.wScan = wbuf[i];
        in[0].ki.dwFlags = KEYEVENTF_UNICODE;
        in[1] = in[0];
        in[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        SendInput(2, in, sizeof(INPUT));
        Sleep(25); // small gap so the PTY keeps up
    }
}

static void pressVk(WORD vk) {
    INPUT in[2] = {};
    in[0].type = INPUT_KEYBOARD; in[0].ki.wVk = vk;
    in[1] = in[0]; in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, in, sizeof(INPUT));
}

int main(int argc, char** argv) {
    SetProcessDPIAware();
    if (argc < 2) { printf("usage: drive <type|enter|maximize|restore|resize> ...\n"); return 2; }

    HWND hwnd = findC11();
    if (!hwnd) { printf("c11 window not found\n"); return 1; }
    printf("c11 hwnd=%p\n", (void*)hwnd);

    const char* cmd = argv[1];
    if (strcmp(cmd, "type") == 0 && argc >= 3) {
        forceForeground(hwnd);
        typeUnicode(argv[2]);
    } else if (strcmp(cmd, "enter") == 0) {
        forceForeground(hwnd);
        pressVk(VK_RETURN);
    } else if (strcmp(cmd, "maximize") == 0) {
        ShowWindow(hwnd, SW_MAXIMIZE);
    } else if (strcmp(cmd, "restore") == 0) {
        ShowWindow(hwnd, SW_RESTORE);
    } else if (strcmp(cmd, "resize") == 0 && argc >= 4) {
        int w = atoi(argv[2]), h = atoi(argv[3]);
        SetWindowPos(hwnd, NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
    } else {
        printf("bad args\n"); return 2;
    }
    return 0;
}
