#include "ScreenSelector.h"
#include "Win32Helper.h"

typedef struct
{
	HDC hDeviceContext;
	HBITMAP hBitmap;
	RECT Rect;
}MyDev; // What to name this? LUL

typedef struct
{
	BOOL Result;
	BOOL Wait;
	MyDev Window;
	MyDev Selection;
}SelectionState;

void CleanUp(const MyDev& dev)
{
	if (dev.hBitmap)
	{
		DeleteObject(dev.hBitmap);
	}

	if (dev.hDeviceContext)
	{
		DeleteDC(dev.hDeviceContext);
	}
}

// TODO(batuhan): You can still exit just by simply changing focus(Alt-tab for example). 
LRESULT CALLBACK SelectionProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	SelectionState *state = (SelectionState *)GetWindowLong(hWnd, GWL_USERDATA);
	static const char* message = "Enter to select - Escape to cancel";
	switch (uMsg)
	{
		case WM_CREATE:
		{
			state = (SelectionState *)((LPCREATESTRUCT)lParam)->lpCreateParams;
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)state);

			SelectObject(state->Selection.hDeviceContext, CreatePen(PS_SOLID, 1, RGB(255, 0, 0)));
			SetROP2(state->Selection.hDeviceContext, R2_NOT);
			//SelectObject(state->Selection.hDeviceContext, (HBRUSH)GetStockObject(NULL_BRUSH));
			SetTextColor(state->Selection.hDeviceContext, (COLORREF)RGB(0, 0, 0));
			SetTextAlign(state->Selection.hDeviceContext, TA_CENTER);
			SetBkMode(state->Selection.hDeviceContext, TRANSPARENT);
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			if (BeginPaint(hWnd, &ps))
			{
				BitBlt(state->Selection.hDeviceContext,
					0, 0, state->Window.Rect.right, state->Window.Rect.bottom,
					state->Window.hDeviceContext,
					0, 0,
					SRCCOPY);

				Rectangle(state->Selection.hDeviceContext,
					state->Selection.Rect.left, state->Selection.Rect.top, state->Selection.Rect.right, state->Selection.Rect.bottom);

				//DrawText(state->Selection.hDeviceContext, message, strlen(message), &state->Selection.Rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				
				// TODO(batuhan): Wrap these around a function; change font, color, etc.
				// TODO(batuhan): Multi-line
				// TODO(batuhan): Make it more readable. 
				int centerX = state->Selection.Rect.left + (state->Selection.Rect.right - state->Selection.Rect.left) / 2;
				int centerY = state->Selection.Rect.top + (state->Selection.Rect.bottom - state->Selection.Rect.top) / 2;
				ExtTextOut(state->Selection.hDeviceContext, centerX, centerY, ETO_CLIPPED, &state->Selection.Rect, message, strlen(message), NULL);

				BitBlt(ps.hdc,
					0, 0, state->Window.Rect.right, state->Window.Rect.bottom,
					state->Selection.hDeviceContext,
					0, 0,
					SRCCOPY);

				EndPaint(hWnd, &ps);
			}
		} break;

		case WM_ERASEBKGND:
		{
			result = 1;
		} break;

		case WM_LBUTTONDOWN:
		{
			SetCapture(hWnd);
			state->Selection.Rect.left = LOWORD(lParam);
			state->Selection.Rect.top = HIWORD(lParam);
		} break;

		case WM_LBUTTONUP:
		{
			ReleaseCapture();
		} break;

		case WM_MOUSEMOVE:
		{
			if (hWnd == GetCapture())
			{
				state->Selection.Rect.right = LOWORD(lParam) + 1;
				state->Selection.Rect.bottom = HIWORD(lParam) + 1;
				RedrawWindow(hWnd, NULL, NULL, RDW_NOERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			}
		} break;

		case WM_KEYDOWN:
		{
			if (wParam == VK_RETURN)
			{
				OffsetRect(&state->Selection.Rect, state->Window.Rect.left, state->Window.Rect.top);
				state->Result = TRUE;
				DestroyWindow(hWnd);
			}
			else if (wParam == VK_ESCAPE)
			{
				state->Result = FALSE;
				DestroyWindow(hWnd);
			}
		} break;

		case WM_CLOSE:
		{
			DestroyWindow(hWnd);
		} break;

		case WM_DESTROY:
		{
			state->Wait = false;
		} break;

		default:
		{
			result = DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}

	return result;
}

bool GetScreenSelection(HINSTANCE hInstance, RECT *rcSelection, HWND target)
{
	char* szClassName = "SelectionClass";
	DWORD style = WS_POPUP | WS_EX_TOPMOST | WS_VISIBLE;
	DWORD styleEx = WS_EX_TOOLWINDOW;
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hInstance = hInstance;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
	wcex.lpfnWndProc = SelectionProc;
	wcex.lpszClassName = szClassName;

	SelectionState state = { 0 };

	HDC hDeviceContext = GetDC(target);
	GetClientRect(target, &state.Window.Rect);

	state.Window.hBitmap = CreateCompatibleBitmap(hDeviceContext, state.Window.Rect.right, state.Window.Rect.bottom);
	if (!state.Window.hBitmap)
		goto cleanup;
	state.Window.hDeviceContext = CreateCompatibleDC(hDeviceContext);
	if (!state.Window.hDeviceContext)
		goto cleanup;

	state.Selection.hBitmap = CreateCompatibleBitmap(hDeviceContext, state.Window.Rect.right, state.Window.Rect.bottom);
	if (!state.Selection.hBitmap)
		goto cleanup;

	state.Selection.hDeviceContext = CreateCompatibleDC(hDeviceContext);
	if (!state.Selection.hDeviceContext)
		goto cleanup;

	SelectObject(state.Window.hDeviceContext, state.Window.hBitmap);
	SelectObject(state.Selection.hDeviceContext, state.Selection.hBitmap);

	BitBlt(state.Window.hDeviceContext,
		0, 0, state.Window.Rect.right, state.Window.Rect.bottom,
		hDeviceContext,
		state.Window.Rect.left, state.Window.Rect.top,
		SRCCOPY);

	ReleaseDC(NULL, hDeviceContext);
	hDeviceContext = NULL;

	HWND hWndSelect = Win32CreateWindow(hInstance, wcex, styleEx, style, "Selection Window",
		state.Window.Rect.right, state.Window.Rect.bottom, state.Window.Rect.left, state.Window.Rect.top, HWND_DESKTOP, &state);

	if (!hWndSelect)
	{
		goto cleanup;
	}

	SetForegroundWindow(target);
	SetForegroundWindow(hWndSelect);

	MSG msg;
	state.Wait = TRUE;
	while (state.Wait)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			PostQuitMessage(msg.wParam);
		}
	}

	if (state.Result)
	{
		CopyRect(rcSelection, &state.Selection.Rect);
	}

cleanup:
	if (hDeviceContext)
		ReleaseDC(NULL, hDeviceContext);

	CleanUp(state.Window);
	CleanUp(state.Selection);

	return state.Result;
};