#include "InputHooks.h"

InputHooks g_inputHooks;

LRESULT WINAPI HookedWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return g_inputHooks.WndProc(hWnd, Msg, wParam, lParam);
}

InputHooks::InputHooks()
    : m_hwnd(NULL)
    , m_originalWndProc(NULL)
{

}

InputHooks::~InputHooks()
{

}

void InputHooks::HookWindow(HWND hWnd)
{
    if (hWnd == m_hwnd)
    {
        return;
    }

    if (m_hwnd)
    {
        RevertHooks();
    }

    m_hwnd = hWnd;
    m_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&HookedWndProc)));
    
}

void InputHooks::RevertHooks()
{
    if (!m_hwnd)
    {
        return;
    }

    SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalWndProc));
    m_originalWndProc = NULL;
    m_hwnd = NULL;
}

LRESULT InputHooks::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (hWnd != m_hwnd)
    {
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    if (GetAsyncKeyState(VK_NUMPAD0) != 0)
    {
        OutputDebugString(L"Ping from hook");
    }

    return CallWindowProc(m_originalWndProc, m_hwnd, Msg, wParam, lParam);
}
