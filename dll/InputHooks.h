#pragma once

#include "common/Platform.h"

#include <mutex>

class InputHooks
{
public:
    InputHooks();
    ~InputHooks();

    void HookWindow(HWND hWnd);
    void RevertHooks();
    LRESULT WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	bool IsCaptureActive();
	void ResetCapture();

private:
	std::mutex m_mutex;
    HWND m_hwnd;
    WNDPROC m_originalWndProc;
	bool m_captureActive;
};

extern InputHooks g_inputHooks;
