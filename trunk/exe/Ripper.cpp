#include "Ripper.h"

#include <algorithm>

#include <TlHelp32.h>

static const UINT_PTR CHECK_PROCESSES_TIMER_ID = 1;
static const UINT CHECK_PROCESSES_TIMER_TIMEOUT = 1000;

LRESULT WINAPI RipperWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_CREATE)
    {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return 0;
    }

    RipperApp* app = reinterpret_cast<RipperApp*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (app)
    {
        return app->WndProc(hWnd, Msg, wParam, lParam);
    }

    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}

RipperApp::RipperApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
: m_hInstance(hInstance)
, m_nCmdShow(nCmdShow)
, m_wndPtr(nullptr, &::DestroyWindow)
{
}

RipperApp::~RipperApp()
{
}

bool RipperApp::Initialize()
{
    m_sharedMemPtr = CreateSharedMemory(FILE_MAPPING_OBJECT_NAME, sizeof(D3D9DeviceOffsets));
    if (!m_sharedMemPtr)
    {
        log(L"Failed to create shared memory\n");
        return false;
    }

    D3D9DeviceOffsets* d3d9DeviceOffsets = new (m_sharedMemPtr.get()) D3D9DeviceOffsets();
    UpdateD3D9Info(m_hInstance, d3d9DeviceOffsets);

    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = &RipperWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = m_hInstance;
    wndClass.hIcon = NULL;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = NULL;
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = L"BerserkRipperWndClass";
    wndClass.hIconSm = NULL;
    RegisterClassEx(&wndClass);

    m_wndPtr.reset(CreateWindow(L"BerserkRipperWndClass", L"BerserkRipper", WS_OVERLAPPEDWINDOW,
                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, m_hInstance, this));

    return bool(m_wndPtr);
}

int RipperApp::Run()
{
    if (!m_wndPtr)
    {
        log(L"Failed to create window\n");
        return 0;
    }

    ShowWindow(m_wndPtr.get(), m_nCmdShow);
   
    SetTimer(m_wndPtr.get(), CHECK_PROCESSES_TIMER_ID, CHECK_PROCESSES_TIMER_TIMEOUT, NULL);

    MSG msg;
    while (GetMessage(&msg, m_wndPtr.get(), 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ClearProcesses();
    return 0;
}

LRESULT RipperApp::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_TIMER:
        if (wParam == CHECK_PROCESSES_TIMER_ID)
        {
            CheckProcesses();
            return 0;
        }
    default:
        break;
    }

    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}

void RipperApp::CheckProcesses()
{
    if (!m_injectedProcesses.empty())
    {
        auto newEndIt = std::remove_if(m_injectedProcesses.begin(), m_injectedProcesses.end(), [](const InjectedProcess& process) -> bool
        {
            DWORD exitCode = 0;
            if (::GetExitCodeProcess(process.hProcess, &exitCode) && exitCode == STILL_ACTIVE)
            {
                return false;
            }
            return true;
        });

        m_injectedProcesses.erase(newEndIt, m_injectedProcesses.end());
    }
    

    std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&::CloseHandle)> pSnapshot(
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0), &::CloseHandle);
    if (!pSnapshot)
    {
        log(L"Failed to enumerate processes\n");
        return;
    }

    PROCESSENTRY32 processInfo = {};
    processInfo.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(pSnapshot.get(), &processInfo) == FALSE)
    {
        log(L"Failed to enumerate processes\n");
        return;
    }

    do
    {
        if (::GetCurrentProcessId() == processInfo.th32ProcessID)
        {
            continue;
        }

        if (!m_injectedProcesses.empty())
        {
            auto itFind = std::find_if(m_injectedProcesses.begin(), m_injectedProcesses.end(), [=](const InjectedProcess& process) -> bool
            {
                return process.pid == processInfo.th32ProcessID;
            });

            if (itFind != m_injectedProcesses.end())
            {
                continue;
            }
        }

        if (CheckModules(processInfo.th32ProcessID))
        {
            if (InjectProcess(processInfo.th32ProcessID))
            {
                log(L"'%s' (%d) injected\n", processInfo.szExeFile, processInfo.th32ProcessID);
            }
            else
            {
                log(L"'%s' (%d) failed to inject\n", processInfo.szExeFile, processInfo.th32ProcessID);
            }
        }
    } while (Process32Next(pSnapshot.get(), &processInfo));
}

bool RipperApp::CheckModules(DWORD pid)
{
    std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&::CloseHandle)> pSnapshot(
        CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid), &::CloseHandle);
    if (!pSnapshot)
    {
        return false;
    }

    MODULEENTRY32 moduleInfo = {};
    moduleInfo.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(pSnapshot.get(), &moduleInfo) == FALSE)
    {
        return false;
    }

    do
    {
        if (_wcsicmp(moduleInfo.szModule, L"d3d9.dll") == 0)
        {
            return true;
        }

    } while (Module32Next(pSnapshot.get(), &moduleInfo));

    return false;
}

bool RipperApp::InjectProcess(DWORD pid)
{
    auto itFind = std::find_if(m_injectedProcesses.cbegin(), m_injectedProcesses.cend(), [=](const InjectedProcess& process) -> bool
    {
        return process.pid == pid;
    });

    if (itFind != m_injectedProcesses.end())
    {
        return true;
    }

    DWORD desiredAccess = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;
    unique_handle_ptr processPtr(OpenProcess(desiredAccess, FALSE, pid), &::CloseHandle);
    if (!processPtr)
    {
        return false;
    }

    DWORD currentDirLen = GetCurrentDirectory(0, NULL);
    std::vector<wchar_t> moduleFullPath(currentDirLen + wcslen(L"\\Ripper.dll"));
    GetCurrentDirectory(currentDirLen, &moduleFullPath[0]);
    wcscat(&moduleFullPath[0], L"\\Ripper.dll");
    
    void* remoteModulePathBuffer = VirtualAllocEx(processPtr.get(), NULL, moduleFullPath.size() * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteModulePathBuffer)
    {
        return false;
    }

    DWORD bytesWritten = 0;
    if (!WriteProcessMemory(processPtr.get(), remoteModulePathBuffer, &moduleFullPath[0], moduleFullPath.size() * sizeof(wchar_t), &bytesWritten))
    {
        return false;
    }

    DWORD threadId = 0;
    unique_handle_ptr threadPtr(CreateRemoteThread(processPtr.get(), NULL, 0, 
                                reinterpret_cast<LPTHREAD_START_ROUTINE>(&::LoadLibrary), 
                                remoteModulePathBuffer, 0, &threadId),
                                &::CloseHandle);
    if (!threadPtr)
    {
        return false;
    }

    WaitForSingleObject(threadPtr.get(), INFINITE);
    DWORD exitCode = 0;
    if (!GetExitCodeThread(threadPtr.get(), &exitCode) || exitCode == 0)
    {
        return false;
    }
    VirtualFreeEx(processPtr.get(), remoteModulePathBuffer, 0, MEM_RELEASE);

    InjectedProcess injectedProcess =
    {
        pid,
        processPtr.release(),
        exitCode
    };
    m_injectedProcesses.push_back(injectedProcess);
    return true;
}

void RipperApp::ClearProcesses()
{
    for (auto it = m_injectedProcesses.begin(), it_end = m_injectedProcesses.end(); it != it_end; ++it)
    {
        InjectedProcess& injectedProcess = (*it);
        DWORD threadId = 0;
        unique_handle_ptr threadPtr(CreateRemoteThread(injectedProcess.hProcess, NULL, 0,
                                    reinterpret_cast<LPTHREAD_START_ROUTINE>(&::FreeLibrary),
                                    reinterpret_cast<LPVOID>(injectedProcess.hInjectedModule), 0, &threadId),
                                    &::CloseHandle);
        if (!threadPtr)
        {
            log(L"Failed to uninject (%d)\n", injectedProcess.pid);
            CloseHandle(injectedProcess.hProcess);
            continue;
        }

        WaitForSingleObject(threadPtr.get(), INFINITE);
        DWORD exitCode = 0;
        GetExitCodeThread(threadPtr.get(), &exitCode);
        CloseHandle(injectedProcess.hProcess);
        log(L"(%d) uninjected with code %d\n", injectedProcess.pid, exitCode);
    }
    m_injectedProcesses.clear();
}
