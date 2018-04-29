#include "resource.h"
#include <windows.h>

#define WM_TRAYICON WM_USER
const char INTERNAL_CLASS[] = "kyC";
const char INTERNAL_WINDO[] = "kyW";
BOOL kyson = FALSE;

void PopUp(HWND wnd) {
	HMENU pu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU));
	POINT pt;
	if (pu && GetCursorPos(&pt)) {
		pu = GetSubMenu(pu, 0);
		if (kyson)
			CheckMenuItem(pu, ID_KEEP, MF_CHECKED | MF_BYCOMMAND);
		TrackPopupMenu(pu, TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
			pt.x, pt.y, 0, wnd, NULL);
	}
}

BOOL SetIcon(HWND wnd, UINT icon) {
	NOTIFYICONDATA nf;
	BOOL rV;
	nf.cbSize = sizeof(nf);
	nf.hWnd = wnd;
	nf.uID = 0;
	nf.uFlags = 0;
	nf.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(icon));
	nf.uFlags = NIF_ICON;

	if (wnd) {
		rV = Shell_NotifyIcon(NIM_MODIFY, &nf);
	}
	else {
		nf.hWnd = CreateWindow(
			INTERNAL_CLASS,
			INTERNAL_WINDO,
			0, 0, 0, 0, 0,
			NULL,
			NULL,
			GetModuleHandle(NULL),
			NULL
		);
		nf.uID = 0;
		nf.uFlags |= NIF_MESSAGE;
		nf.uCallbackMessage = WM_TRAYICON;
		rV = Shell_NotifyIcon(NIM_ADD, &nf);
	}
	if (icon) 
		DestroyIcon(nf.hIcon);
	return rV;
}

void DeleteIcon(HWND wnd) {
	NOTIFYICONDATA nf;
	nf.cbSize = sizeof(NOTIFYICONDATA);
	nf.hWnd = wnd;
	nf.uID = 0;
	nf.uFlags = 0;
	Shell_NotifyIcon(NIM_DELETE, &nf);
}

void SwitchKyson(HWND wnd) {
	if (kyson) {
		SetThreadExecutionState(ES_CONTINUOUS);
		SetIcon(wnd, IDI_OFF);
		kyson = FALSE;
	}
	else {
		SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
		SetIcon(wnd, IDI_SCREEN);
		kyson = TRUE;
	}
}

LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
	switch (msg)
	{
	case WM_CREATE:
		break;
	case WM_COMMAND:
		switch (w) {
		case ID_KEEP:
			SwitchKyson(wnd);
			break;
		case ID_QUIT:
			DestroyWindow(wnd);
			break;
		}
		break;
	case WM_TRAYICON:
		switch (l) {
		case WM_LBUTTONUP:
			SwitchKyson(wnd);
			break;
		case WM_RBUTTONUP:
			PopUp(wnd);
			break;
		}
		return TRUE;
	case WM_DESTROY:
		if (kyson) SwitchKyson(wnd);
		DeleteIcon(wnd);
		PostQuitMessage(0);
		break;

	}
	return DefWindowProc(wnd, msg, w, l);
}

int main() {
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = INTERNAL_CLASS;
	if (RegisterClass(&wc)) {
		kyson = FALSE;
		if (SetIcon(NULL, IDI_OFF)) {
			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	return 0;
}