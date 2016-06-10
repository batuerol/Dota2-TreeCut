#include <iostream>
#include <cmath>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

#include <Windows.h>
#include <CommCtrl.h>

#include "Resources.h"
#include "Win32Helper.h"
#include "ScreenSelector.h"

//TODO(batuhan): Wrap process searcher in a class...
//TODO(batuhan): g_Running needs to be guarded. Since thread(s) depends on it.
//TODO(batuhan): Keyboard hook lags the whole system. Probaby because of g_Running. DEBUG

static const char* g_szTargetWindow = "Dota 2";
static bool g_Running = true;
static bool g_Cut = false;

static HINSTANCE g_hInstance;
static HANDLE g_hWait = NULL;
static HWND g_hWndDota = NULL;

static RECT g_DotaRect;
static LPARAM g_Position;

static std::condition_variable g_WaitProcessCV;
static std::mutex g_WaitProcessMutex;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *kbStruct = (KBDLLHOOKSTRUCT*)(lParam);
	if (wParam == WM_KEYUP)
	{
		if (kbStruct->vkCode == VK_F10)
		{
			g_Running = false;
			std::cout << "TRIGGERED" << std::endl;
		}
		else if (kbStruct->vkCode == VK_F9)
		{
			g_Cut = !g_Cut;
		}
	}
	return 0;
}

VOID CALLBACK WaitOrTimerCallback(PVOID lpParam, BOOLEAN timerOrWaitFired)
{
	std::lock_guard<std::mutex> lock(g_WaitProcessMutex);
	g_hWndDota = NULL;
	g_WaitProcessCV.notify_one();
}

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	static HICON hIcDotaOk = (HICON)LoadImage(g_hInstance, "..\\res\\seemsgood.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
	static HICON hIcDotaNo = (HICON)LoadImage(g_hInstance, "..\\res\\biblethump.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);

	switch (uMsg)
	{
		case WM_CREATE:
		{
			//HFONT font = Win32MakeFont("Tahoma", 10, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET);
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);
			HWND text = CreateWindowEx(
				NULL,
				WC_STATIC,
				"",
				WS_CHILD | WS_VISIBLE | SS_CENTER,
				5, 10,
				80, 20,
				hWnd, NULL,
				g_hInstance,
				NULL);

			//SendMessage(text, WM_SETFONT, (WPARAM)font, 0);
			SendMessage(text, WM_SETTEXT, 0, (LPARAM)"Dota status:");

			HWND dotaStatus = CreateWindowEx(
				NULL,
				WC_STATIC,
				"",
				WS_CHILD | WS_VISIBLE | SS_ICON,
				90, 0,
				10, 10,
				hWnd, NULL,
				g_hInstance,
				NULL);

			SendMessage(dotaStatus, STM_SETIMAGE, IMAGE_ICON, (WPARAM)hIcDotaOk);

			HWND toggleCut = CreateWindowEx(
				NULL,
				WC_BUTTON,
				"",
				WS_CHILD | WS_VISIBLE | SS_CENTER | BS_AUTOCHECKBOX,
				rcClient.right - 100, rcClient.bottom - 20,
				100, 20,
				hWnd, (HMENU)IDC_TOGGLECUT,
				g_hInstance, NULL);
			//SendMessage(toggleCut, WM_SETFONT, (WPARAM)font, 0);
			SendMessage(toggleCut, WM_SETTEXT, 0, (LPARAM)"Toggle Cut");

			HWND changeArea = CreateWindowEx(
				NULL,
				WC_BUTTON,
				"",
				WS_CHILD | WS_VISIBLE | SS_CENTER,
				rcClient.left + 10, rcClient.top + 100,
				100, 20,
				hWnd, (HMENU)IDC_AREASELECT,
				g_hInstance, NULL);
			SendMessage(changeArea, WM_SETTEXT, 0, (LPARAM)"Area Select");

		} break;

		case WM_CTLCOLORSTATIC:
		{
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
			SetBkMode((HDC)wParam, RGB(255, 0, 0));
			result = (LRESULT)GetStockObject(WHITE_BRUSH);
		}

		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_AREASELECT)
			{
				RECT rect = { 0 };
				//if (GetScreenSelection(g_hInstance, &rect, g_hWndDota))
				{
					CHAR szText[128];
					wsprintf(szText, "(%d, %d) - (%d, %d)", rect.left, rect.top, rect.right, rect.bottom);
					MessageBox(HWND_DESKTOP, szText, "Selected Rectangle", MB_OK | MB_SETFOREGROUND);
				}
			}
		} break;

		case WM_CLOSE:
		case WM_DESTROY:
		{
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			g_Running = false;
		} break;

		case WM_QUIT:
		{
			g_Running = false;
		} break;

		default:
		{
			result = DefWindowProcA(hWnd, uMsg, wParam, lParam);
		} break;
	}

	return result;
}

void cutThread()
{
	const int left = 295;
	const int top = 268;
	const int right = 1260;
	const int bottom = 865;
	while (g_Running)
	{
		if (g_Cut)
		{
			for (int x = left; x <= right; x += 30)
			{
				for (int y = top; y <= bottom; y += 50)
				{
					//MoveMouseRelative(g_DotaRect, x, y);
					//SendClick(g_TargetWindow ,x, y);
					Win32PostClick(g_hWndDota, x, y);
					Sleep(1);
					//std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				Sleep(30);
				//std::this_thread::sleep_for(std::chrono::milliseconds(30));
			}
		}
		else
		{
			// TODO(batuhan): Delays program closing.
			//std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}

void SearchDota()
{
	while (g_Running)
	{
		std::unique_lock<std::mutex> lock(g_WaitProcessMutex);
		// NOTE(batuhan): Wait till dota != null
		g_WaitProcessCV.wait(lock, [] { return g_hWndDota == NULL; });

		// NOTE(batuhan): Early exit.
		if (!g_Running)
		{
			break;
		}

		g_hWndDota = FindWindow(NULL, g_szTargetWindow);
		if (g_hWndDota != NULL)
		{
			if (g_hWait == NULL)
			{
				Win32RegisterCloseHook(g_hWndDota, WaitOrTimerCallback, &g_hWait);
			}
			lock.unlock();
			g_WaitProcessCV.notify_one();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	g_hInstance = hInstance;
	WNDCLASSEX mainWndClass = { 0 };
	mainWndClass.cbSize = sizeof(WNDCLASSEX);
	mainWndClass.style = CS_VREDRAW | CS_HREDRAW;
	mainWndClass.hInstance = hInstance;
	mainWndClass.hbrBackground = (HBRUSH)GetStockObject(DC_BRUSH);
	mainWndClass.lpszClassName = "CUTWindowClass";
	mainWndClass.lpfnWndProc = MainWindowProc;
	DWORD mainStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | !WS_MAXIMIZEBOX | WS_VISIBLE;

	HWND hMainWnd = Win32CreateWindow(hInstance, mainWndClass, 0, mainStyle, "Cut cut cut ccccccuut", 400, 400);
	if (hMainWnd == 0)
	{
		std::cerr << "Window creation failed. Error code:" << GetLastError() << std::endl;
		return 1;
	}

	HHOOK hKeyboardHook = Win32CreateKeyboardHook(LowLevelKeyboardProc);
	if (hKeyboardHook == 0)
	{
		std::cerr << "Hook creation failed. Error code:" << GetLastError() << std::endl;
		return 1;
	}

	//g_hWndDota = FindWindow(NULL, "Dota 2");
	//if (g_hWndDota != NULL)
	//{
	//	GetClientRect(g_hWndDota, &g_DotaRect);
	//	//SetForegroundWindow(g_TargetWindow);
	//}
	//Sleep(100);

	//std::thread cutter(cutThread);
	std::thread processSearcher(SearchDota);
	MSG message;
	while (g_Running)
	{
		while (PeekMessage(&message, hMainWnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		//if (GetAsyncKeyState(VK_F10))
		//{
		//	g_Running = false;
		//	//PostQuitMessage(0);
		//	//DestroyWindow(hMainWnd);			
		//}

		//if (GetAsyncKeyState(VK_F9))
		//{
		//	g_Cut = !g_Cut;
		//}
	}
	//cutter.join();
	g_hWndDota = NULL;
	g_WaitProcessCV.notify_all();
	processSearcher.join();

	return 0;
}