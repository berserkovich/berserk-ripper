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
    std::lock_guard<std::mutex> lock(m_mutex);

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
    std::lock_guard<std::mutex> lock(m_mutex);

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
    HWND thisHwnd;
    WNDPROC thisWndProc;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        thisHwnd = m_hwnd;
        thisWndProc = m_originalWndProc;
    }

    if (hWnd != thisHwnd)
    {
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    if (GetAsyncKeyState(VK_NUMPAD0) != 0)
    {
        OutputDebugString(L"Capture active");
        std::lock_guard<std::mutex> lock(m_mutex);
        m_captureActive = true;
    }

    return CallWindowProc(thisWndProc, thisHwnd, Msg, wParam, lParam);
}

bool InputHooks::IsCaptureActive()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_captureActive;
}

void InputHooks::ResetCapture()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_captureActive = false;
}
