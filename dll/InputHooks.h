#pragma once

#include "common/Platform.h"

class InputHooks
{
public:
    InputHooks();
    ~InputHooks();

    void HookWindow(HWND hWnd);
    void RevertHooks();
    LRESULT WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd;
    WNDPROC m_originalWndProc;
};

extern InputHooks g_inputHooks;
