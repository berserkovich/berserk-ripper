#pragma once

#include "common/Common.h"
#include "common/Platform.h"

#include <memory>

void UpdateD3D9Info(HINSTANCE _hInst, D3D9DeviceOffsets* d3d9DeviceOffsets);

class RipperApp
{
public:
    RipperApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);
    ~RipperApp();

    bool Initialize();
    int Run();
    LRESULT WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
    typedef std::unique_ptr<std::remove_pointer<HWND>::type, decltype(&::DestroyWindow)> unique_wnd_ptr;

private:
    const HINSTANCE m_hInstance;
    const int m_nCmdShow;

    unique_sharedmem_ptr m_sharedMemPtr;
    unique_wnd_ptr m_wndPtr;
};
