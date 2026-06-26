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

// Click at a point given in CLIENT coordinates of the c11 window.
static void clickClient(HWND hwnd, int cx, int cy) {
    forceForeground(hwnd);
    POINT pt{cx, cy};
    ClientToScreen(hwnd, &pt);
    SetCursorPos(pt.x, pt.y);
    Sleep(60);
    INPUT in[2] = {};
    in[0].type = INPUT_MOUSE; in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    in[1].type = INPUT_MOUSE; in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, in, sizeof(INPUT));
}

static void pressVk(WORD vk) {
    INPUT in[2] = {};
    in[0].type = INPUT_KEYBOARD; in[0].ki.wVk = vk;
    in[1] = in[0]; in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, in, sizeof(INPUT));
}

// Send a modifier chord, e.g. Ctrl+Shift+M. mods is a string of c/s/a (ctrl/
// shift/alt); key is a single character. Holds modifiers, taps the key, releases.
static void sendChord(const char* mods, char key) {
    WORD mvk[3]; int n = 0;
    for (const char* p = mods; *p; ++p) {
        if (*p == 'c') mvk[n++] = VK_CONTROL;
        else if (*p == 's') mvk[n++] = VK_SHIFT;
        else if (*p == 'a') mvk[n++] = VK_MENU;
    }
    INPUT in = {}; in.type = INPUT_KEYBOARD;
    for (int i = 0; i < n; ++i) { in.ki.wVk = mvk[i]; in.ki.dwFlags = 0; SendInput(1, &in, sizeof(INPUT)); }
    WORD kvk = VkKeyScanA(key) & 0xFF;
    in.ki.wVk = kvk; in.ki.dwFlags = 0; SendInput(1, &in, sizeof(INPUT));
    in.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &in, sizeof(INPUT));
    for (int i = n - 1; i >= 0; --i) { in.ki.wVk = mvk[i]; in.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &in, sizeof(INPUT)); }
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
    } else if (strcmp(cmd, "click") == 0 && argc >= 4) {
        clickClient(hwnd, atoi(argv[2]), atoi(argv[3]));
    } else if (strcmp(cmd, "chord") == 0 && argc >= 4) {
        forceForeground(hwnd);
        sendChord(argv[2], argv[3][0]); // e.g. drive chord cs m  -> Ctrl+Shift+M
    } else if (strcmp(cmd, "rawtype") == 0 && argc >= 3) {
        typeUnicode(argv[2]); // type into whatever is focused (e.g. a dialog)
    } else if (strcmp(cmd, "rawenter") == 0) {
        pressVk(VK_RETURN);
    } else {
        printf("bad args\n"); return 2;
    }
    return 0;
}
