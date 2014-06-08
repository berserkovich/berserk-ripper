#include "Ripper.h"

#include <algorithm>

static const UINT_PTR CHECK_PROCESSES_TIMER_ID = 1;
static const UINT CHECK_PROCESSES_TIMER_TIMEOUT = 1000;

static const wchar_t* ProcessExclusionList[] = {
    L"chrome.exe",
    L"skype.exe"
};

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
, m_sharedMutex(nullptr, &::CloseHandle)
, m_wndPtr(nullptr, &::DestroyWindow)
{
}

RipperApp::~RipperApp()
{
    if (m_sharedMutex)
    {
        ReleaseMutex(m_sharedMutex.get());
    }
}

bool RipperApp::Initialize()
{
    m_sharedMutex.reset(CreateMutex(NULL, TRUE, SHARED_MUTEX_NAME));
    if (!m_sharedMutex)
    {
        log(L"Failed to create mutex");
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        log(L"Mutex is locked");
        return false;
    }

    m_sharedMemPtr = CreateSharedMemory(FILE_MAPPING_OBJECT_NAME, sizeof(SharedData));
    if (!m_sharedMemPtr)
    {
        log(L"Failed to create shared memory\n");
        return false;
    }

    SharedData* sharedData = new (m_sharedMemPtr.get()) SharedData();
    UpdateD3D9Info(m_hInstance, &sharedData->d3d9DeviceOffsets);

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
        if (FilterProcess(processInfo))
        {
            continue;
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

    bool injectTarget = false;

    do
    {
        if (_wcsicmp(moduleInfo.szModule, L"ripper.dll") == 0)
        {
            return false;
        }

        if (_wcsicmp(moduleInfo.szModule, L"d3d9.dll") == 0)
        {
            injectTarget = true;
        }

    } while (Module32Next(pSnapshot.get(), &moduleInfo));

    return injectTarget;
}

bool RipperApp::InjectProcess(DWORD pid)
{
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

    return true;
}

bool RipperApp::FilterProcess(const PROCESSENTRY32& processInfo)
{
    if (::GetCurrentProcessId() == processInfo.th32ProcessID)
    {
        return true;
    }

    auto itEnd = ProcessExclusionList + sizeof(ProcessExclusionList) / sizeof(const wchar_t*);
    auto itFind = std::find_if(ProcessExclusionList, itEnd, [&](const wchar_t* processName)
    {
        return _wcsicmp(processName, processInfo.szExeFile) == 0;
    });

    return itFind != itEnd;
}
