#include "resource.h"

#include <Windows.h>
#include <Wtsapi32.h>
#include <WinUser.h>

class Kyson {

public:
    Kyson(
        bool busy,
        bool busy_when_locked,
        bool off_when_locked
    ) : busy(busy),
        busy_when_locked(busy_when_locked),
        off_when_locked(off_when_locked)
    {
        nf.cbSize = sizeof(nf);
        nf.uID = 0;
        module = GetModuleHandle(NULL);
    }

    LRESULT callback(HWND wnd, UINT msg, WPARAM w, LPARAM l) {
        switch (msg)
        {
        case WM_CREATE:
            break;
        case WM_WTSSESSION_CHANGE:
            switch (w) {
            case WTS_SESSION_LOCK:
                if (!busy_when_locked && busy)
                    screenOff();
                if (off_when_locked)
                    SendMessage(wnd, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
                break;
            case WTS_SESSION_UNLOCK:
                if (!busy_when_locked && busy)
                    screenOn();
                break;
            }
            break;
        case WM_COMMAND:
            switch (w) {
            case ID_KEEP_WHEN_LOCKED:
                busy_when_locked = !busy_when_locked;
                break;
            case ID_LOCK:
                off_when_locked = !off_when_locked;
                break;
            case ID_KEEP:
                busy = !busy;
                screenImplement();
                break;
            case ID_QUIT:
                DestroyWindow(wnd);
                break;
            }
            break;
        case WM_USER:
            switch (l) {
            case WM_LBUTTONUP:
                busy = !busy;
                screenImplement();
                break;
            case WM_RBUTTONUP:
                openMenu();
                break;
            }
            return TRUE;
        case WM_DESTROY:
            screenOff();
            deleteIcon();
            PostQuitMessage(0);
            break;

        }
        return DefWindowProc(wnd, msg, w, l);
    }

    static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
    {
        Kyson *ksn = reinterpret_cast<Kyson*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
        return ksn 
            ? ksn->callback(wnd, msg, w, l)
            : DefWindowProc(wnd, msg, w, l);
    }

    bool start() {
        WNDCLASS wc = { 0 };
        bool success = false;
        wc.lpfnWndProc = WndProc;
        wc.lpszClassName = windowClass;
        if (RegisterClass(&wc)) {
            nf.hWnd = CreateWindow(
                windowClass,
                windowName,
                0, 0, 0, 0, 0,
                NULL,
                NULL,
                module,
                NULL
            );
            if (nf.hWnd) {
                WTSRegisterSessionNotification(nf.hWnd, NOTIFY_FOR_THIS_SESSION);
                SetLastError(0);
                if (SetWindowLongPtr(nf.hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this))
                    || !GetLastError())
                {
                    nf.uFlags = NIF_MESSAGE;
                    nf.uCallbackMessage = windowMessage;
                    if (Shell_NotifyIcon(NIM_ADD, &nf)) {
                        MSG msg;
                        success = true;
                        screenImplement();
                        while (GetMessage(&msg, NULL, 0, 0)) {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }
                }
                WTSUnRegisterSessionNotification(nf.hWnd);
            }
        }
        return false;
    }

private:
    void screenImplement() {
        setIcon();
        if (busy) screenOn();
        else screenOff();
    }

    void screenOn() {
        SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
    }
    void screenOff() {
        SetThreadExecutionState(ES_CONTINUOUS);
    }

    bool setIcon() {
        nf.uFlags = NIF_ICON;
        nf.hIcon = LoadIcon(module, MAKEINTRESOURCE(busy ? IDI_SCREEN : IDI_OFF));
        bool success = !!Shell_NotifyIcon(NIM_MODIFY, &nf);
        DestroyIcon(nf.hIcon);
        return success;
    }

    void deleteIcon() {
        nf.uFlags = 0;
        Shell_NotifyIcon(NIM_DELETE, &nf);
    }

    void openMenu() {
        HMENU pu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU));
        POINT pt;
        if (pu && GetCursorPos(&pt)) {
            pu = GetSubMenu(pu, 0);
            if (busy) 
                CheckMenuItem(pu, ID_KEEP, MF_CHECKED | MF_BYCOMMAND);
            if (busy_when_locked)
                CheckMenuItem(pu, ID_KEEP_WHEN_LOCKED, MF_CHECKED | MF_BYCOMMAND);
            if (off_when_locked) 
                CheckMenuItem(pu, ID_LOCK, MF_CHECKED | MF_BYCOMMAND);

            if (SetForegroundWindow(nf.hWnd)) {
                TrackPopupMenu(pu, TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
                    pt.x, pt.y, 0, nf.hWnd, NULL);
            }
        }
    }

private:
    const static char windowClass[];
    const static char windowName[];
    const static UINT windowMessage = WM_USER;

private:
    bool busy;
    bool busy_when_locked;
    bool off_when_locked;
    HMODULE module;
    NOTIFYICONDATA nf;
};

const char Kyson::windowClass[] = "kyC";
const char Kyson::windowName[]  = "kyW";

void __declspec(noreturn) main() {
    int k, argc;
    bool busy = true;
    bool busy_while_locked = false;
    bool off_when_locked = true;

    WCHAR **argv = CommandLineToArgvW(
        GetCommandLineW(), &argc);

    for (k = 0; k < argc; k++) {
        /* if this is a one letter flag: */
        if (argv[k][0] == '/' && argv[k][3] == 0) {
            switch (argv[k][1]) {
            case 'B':
                busy = false;
                break;
            case 'L':
                busy_while_locked = true;
                break;
            case '0':
                off_when_locked = false;
                break;
            }
        }
    }

    Kyson K(busy, busy_while_locked, off_when_locked);

    if (K.start())
        ExitProcess(0);
    else
        ExitProcess(1);
}