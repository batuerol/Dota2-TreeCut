#include "Win32Helper.h"

LPARAM Win32MakeMousePos(int x, int y)
{
	return MAKELPARAM(x, y);
}

POINT Win32GetCursorPosition()
{
	POINT p;
	GetCursorPos(&p);
	return p;
}

void Win32MoveMouseRelative(RECT clientRect, int x, int y)
{
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.dx = (clientRect.left + x) * (0xFFFF / (clientRect.right - clientRect.left));
	input.mi.dy = (clientRect.top + y) * (0xFFFF / (clientRect.bottom - clientRect.top));
	input.mi.dwFlags = MOUSEEVENTF_MOVE_NOCOALESCE | MOUSEEVENTF_ABSOLUTE;

	SendInput(1, &input, sizeof(INPUT));
}

void Win32SendClick(HWND hWnd, int x, int y, int repeat)
{
	LPARAM lParam = Win32MakeMousePos(x, y);
	SendMessage(hWnd, WM_MOUSEMOVE, 0, lParam);
	for (int i = 0; i < repeat; ++i)
	{
		SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
		SendMessage(hWnd, WM_RBUTTONDOWN, MK_RBUTTON, lParam);
		SendMessage(hWnd, WM_LBUTTONUP, 0, lParam);
		SendMessage(hWnd, WM_RBUTTONUP, 0, lParam);
	}
}

void Win32PostClick(HWND hWnd, int x, int y, int repeat)
{
	LPARAM lParam = Win32MakeMousePos(x, y);
	PostMessage(hWnd, WM_MOUSEMOVE, 0, lParam);
	for (int i = 0; i < repeat; ++i)
	{
		PostMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
		PostMessage(hWnd, WM_RBUTTONDOWN, MK_RBUTTON, lParam);
		PostMessage(hWnd, WM_LBUTTONUP, 0, lParam);
		PostMessage(hWnd, WM_RBUTTONUP, 0, lParam);
	}
}

HFONT Win32MakeFont(const char* fontName, int height, int weight, BOOL italic, BOOL underline, BOOL strike, int charSet)
{
	HDC hdc = GetDC(HWND_DESKTOP);
	height = -MulDiv(height, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(HWND_DESKTOP, hdc);
	return CreateFont(height, 0, 0, 0, weight, italic, underline, strike, charSet, OUT_STROKE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, fontName);
}

HWND Win32CreateWindow(HINSTANCE hInstance, WNDCLASSEX &wndClass, const DWORD& styleEx, const DWORD &style, const char* Title,
	int Width, int Height, int X, int Y,
	HWND Parent, LPVOID lpParam)
{
	// TODO(batuhan): Maybe unregister and re-register for the changes in window class?
	if (!GetClassInfoEx(hInstance, wndClass.lpszClassName, &wndClass))
	{
		if (RegisterClassEx(&wndClass) == 0)
		{
			OutputDebugStringA("RegisterClassEx is an ass. GabeN\n");
			return NULL;
		}
	}

	HWND hWnd = CreateWindowEx(
		styleEx,
		wndClass.lpszClassName,
		Title,
		style,
		X, Y,
		Width, Height,
		Parent, 0, hInstance, lpParam);

	return hWnd;
}

HHOOK Win32CreateKeyboardHook(const HOOKPROC &hookProc)
{
	return SetWindowsHookEx(WH_KEYBOARD_LL, hookProc, 0, 0);
}