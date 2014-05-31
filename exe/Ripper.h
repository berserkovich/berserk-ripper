#pragma once

#include "common/Common.h"
#include "common/Platform.h"

#include <memory>
#include <vector>

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
    void CheckProcesses();
    bool CheckModules(DWORD pid);
    bool InjectProcess(DWORD pid);
    void ClearProcesses();

private:
    typedef std::unique_ptr<std::remove_pointer<HWND>::type, decltype(&::DestroyWindow)> unique_wnd_ptr;
    typedef std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&::CloseHandle)> unique_handle_ptr;

    struct InjectedProcess
    {
        DWORD pid;
        HANDLE hProcess;
        DWORD hInjectedModule;
    };

private:
    const HINSTANCE m_hInstance;
    const int m_nCmdShow;

    unique_sharedmem_ptr m_sharedMemPtr;
    unique_wnd_ptr m_wndPtr;

    std::vector<InjectedProcess> m_injectedProcesses;
};
