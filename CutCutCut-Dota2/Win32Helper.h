#pragma once

#include <Windows.h>

LPARAM Win32MakeMousePos(int x, int y);
POINT Win32GetCursorPosition();
void Win32MoveMouseRelative(RECT clientRect, int x, int y);
void Win32SendClick(HWND hWnd, int x, int y, int repeat = 1);
void Win32PostClick(HWND hWnd, int x, int y, int repeat = 1);
HFONT Win32MakeFont(const char* fontName, int height, int weight, BOOL italic, BOOL underline, BOOL strike, int charSet);
HWND Win32CreateWindow(HINSTANCE hInstance, WNDCLASSEX &wndClass, const DWORD& styleEx, const DWORD &style, const char* Title, int Width = CW_USEDEFAULT, int Height = CW_USEDEFAULT, int X = CW_USEDEFAULT, int Y = CW_USEDEFAULT, HWND Parent = 0, LPVOID lpParam = 0);
HHOOK Win32CreateKeyboardHook(const HOOKPROC &hookProc);
bool Win32RegisterCloseHook(HWND hWnd, WAITORTIMERCALLBACK callback, HANDLE *waitHandle);