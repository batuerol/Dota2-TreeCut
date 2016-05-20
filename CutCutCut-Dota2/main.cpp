#include <windows.h>
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <cmath>
#include <thread>

//static const char* g_TargetWindow = "Dota 2";
static const float TWO_TIMES_PI = 3.14159265358979323846 * 2;
static bool g_Running = true;
static HWND g_TargetWindow;
static RECT g_DotaRect;

void MoveMouse(int x, int y)
{
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dx = (g_DotaRect.left + x) * (0xFFFF / (g_DotaRect.right - g_DotaRect.left));
	input.mi.dy = (g_DotaRect.top + y) * (0xFFFF / (g_DotaRect.bottom - g_DotaRect.top));
	input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;// | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_RIGHTDOWN;
	
	SendInput(1, &input, sizeof(INPUT));
}

void SendClick()
{
	POINT p;
	GetCursorPos(&p);
	LPARAM lParam = MAKELPARAM(p.x, p.y);
	SendMessage(g_TargetWindow, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
	SendMessage(g_TargetWindow, WM_RBUTTONDOWN, MK_RBUTTON, lParam);
	SendMessage(g_TargetWindow, WM_LBUTTONUP, MK_LBUTTON, lParam);
	SendMessage(g_TargetWindow, WM_RBUTTONUP, MK_RBUTTON, lParam);
}

void ClickerThreadFunc()
{
	while(g_Running)
	{
		SendClick();
	}
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *kbStruct = (KBDLLHOOKSTRUCT*)(lParam);
	if (kbStruct->vkCode == VK_ESCAPE && wParam == WM_KEYUP)
	{
		g_Running = false;
		std::cout << "TRIGGERED" << std::endl;
	}

	return 0;
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (uMsg)
	{
		case WM_QUIT:
		case WM_CLOSE:
		case WM_DESTROY:
		{
			g_Running = false;
		} break;

		default:
		{
			result = DefWindowProcA(hwnd, uMsg, wParam, lParam);
		} break;
	}

	return result;
}

HWND CreateWin32Window(HINSTANCE hInstance)
{
	WNDCLASSA wndClass = { 0 };
	wndClass.style = CS_VREDRAW | CS_HREDRAW;
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = "CUTWindowClass";
	wndClass.lpfnWndProc = &MainWindowProc;

	if (RegisterClassA(&wndClass) == 0)
	{
		std::cerr << "RegisterClassEx is an ass." << std::endl;
		return false;
	}

	HWND hWnd = CreateWindowExA(
		0,
		wndClass.lpszClassName,
		"Cut cut cut cccccut",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0, 0, hInstance, 0);

	return hWnd;
}

HHOOK CreateKeyboardHook()
{
	return SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, 0, 0);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd = CreateWin32Window(hInstance);
	if (hWnd == 0)
	{
		std::cerr << "Window creation failed. Error code:" << GetLastError() << std::endl;
		return 1;
	}
	HHOOK hKeyboardHook = CreateKeyboardHook();

	if (hKeyboardHook == 0)
	{
		std::cerr << "Hook creation failed. Error code:" << GetLastError() << std::endl;
		return 1;
	}

	g_TargetWindow = FindWindow(NULL, "Dota 2");
	GetClientRect(g_TargetWindow, &g_DotaRect);		
	SetForegroundWindow(g_TargetWindow);
	Sleep(100);

	//std::thread clicker(ClickerThreadFunc);
	MSG message;
	while (g_Running)
	{
		while (PeekMessage(&message, hWnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		// TODO(batuhan): Clicker thread		
		MoveMouse(476, 705);
		SendClick();		

		const int a = 767;
		const int b = 515;
		const int r = 241;		
		//for (float i = 0.0f; i < TWO_TIMES_PI; i += 0.25f)
		//{
		//	int x = a + (r * cos(i));
		//	int y = b + (r * sin(i));
		//	MoveMouse(x, y);
		//	SendClick();
		//}

		const int left = 295;
		const int top = 268;
		const int right = 1260;
		const int bottom = 865;
		
		//for (int x = left; x <= right; x += 30)
		//{
		//	for (int y = top; y <= bottom; y += 50)
		//	{
		//		MoveMouse(x, y);
		//		SendClick();
		//	}
		//}
		//Sleep(1000);
	}
	
	//clicker.join();

	return 0;
}