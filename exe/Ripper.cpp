#include "Ripper.h"

LRESULT WINAPI RipperWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_CREATE)
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
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
        return 0;
    }

    ShowWindow(m_wndPtr.get(), m_nCmdShow);

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
    default:
        break;
    }

    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}
