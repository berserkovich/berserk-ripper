#include "Ripper.h"

#include "common/Platform.h"

#define CINTERFACE
#include <d3d9.h>

#include <cassert>
#include <memory>

typedef IDirect3D9 * (WINAPI* PDIRECT3DCREATE9)(UINT SDKVersion);

typedef std::unique_ptr<std::remove_pointer<HWND>::type, decltype(&::DestroyWindow)> unique_wnd_ptr;
typedef std::unique_ptr<std::remove_pointer<HMODULE>::type, decltype(&::FreeLibrary)> unique_module_ptr;

void UpdateD3D9Info(HINSTANCE _hInst, D3D9DeviceOffsets* d3d9DeviceOffsets)
{
    assert(d3d9DeviceOffsets);

    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = &DefWindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = _hInst;
    wndClass.hIcon = NULL;
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = NULL;
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = L"BerserkRipperD3D9TestClass";
    wndClass.hIconSm = NULL;
    RegisterClassEx(&wndClass);

    unique_wnd_ptr pWnd(CreateWindow(L"BerserkRipperD3D9TestClass", L"BerserkRipperD3D9TestWindow", WS_OVERLAPPEDWINDOW, 0, 0, 
                        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, _hInst, NULL),
                        &::DestroyWindow);
    if (!pWnd)
    {
        return;
    }

    unique_module_ptr pD3D9Dll(::LoadLibraryA("d3d9.dll"), &::FreeLibrary);
    if (!pD3D9Dll)
    {
        return;
    }

    PDIRECT3DCREATE9 pDirect3DCreate9 = reinterpret_cast<PDIRECT3DCREATE9>(::GetProcAddress(pD3D9Dll.get(), "Direct3DCreate9"));
    if (!pDirect3DCreate9)
    {
        return;
    }

    IDirect3D9* pDirect3D9 = pDirect3DCreate9(D3D_SDK_VERSION);
    if (!pDirect3D9)
    {
        return;
    }
    auto d3d9Deleter = [](IDirect3D9* _d3d9){ _d3d9->lpVtbl->Release(_d3d9); };
    std::unique_ptr<IDirect3D9, decltype(d3d9Deleter)> pD3D9(pDirect3D9, d3d9Deleter);

    D3DDISPLAYMODE displayMode = {};
    HRESULT hr = pD3D9->lpVtbl->GetAdapterDisplayMode(pD3D9.get(), D3DADAPTER_DEFAULT, &displayMode);
    if (FAILED(hr))
    {
        return;
    }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = displayMode.Format;

    IDirect3DDevice9* pD3D9Device = NULL;
    hr = pD3D9->lpVtbl->CreateDevice(pD3D9.get(), D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pWnd.get(),
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
                                    &d3dpp, &pD3D9Device);
    if (FAILED(hr) || pD3D9Device == NULL)
    {
        return;
    }
    auto d3dDeviceDeleter = [](IDirect3DDevice9* _d3d9Device){ _d3d9Device->lpVtbl->Release(_d3d9Device); };
    std::unique_ptr<IDirect3DDevice9, decltype(d3dDeviceDeleter)> pDevice(pD3D9Device, d3dDeviceDeleter);

    d3d9DeviceOffsets->DrawIndexedPrimitive = reinterpret_cast<unsigned char*>(pDevice->lpVtbl->DrawIndexedPrimitive) - reinterpret_cast<unsigned char*>(pD3D9Dll.get());
    d3d9DeviceOffsets->DrawIndexedPrimitiveUP = reinterpret_cast<unsigned char*>(pDevice->lpVtbl->DrawIndexedPrimitiveUP) - reinterpret_cast<unsigned char*>(pD3D9Dll.get());
    d3d9DeviceOffsets->DrawPrimitive = reinterpret_cast<unsigned char*>(pDevice->lpVtbl->DrawPrimitive) - reinterpret_cast<unsigned char*>(pD3D9Dll.get());
    d3d9DeviceOffsets->DrawPrimitiveUP = reinterpret_cast<unsigned char*>(pDevice->lpVtbl->DrawPrimitiveUP) - reinterpret_cast<unsigned char*>(pD3D9Dll.get());
    d3d9DeviceOffsets->DrawRectPatch = reinterpret_cast<unsigned char*>(pDevice->lpVtbl->DrawRectPatch) - reinterpret_cast<unsigned char*>(pD3D9Dll.get());
    d3d9DeviceOffsets->DrawTriPatch = reinterpret_cast<unsigned char*>(pDevice->lpVtbl->DrawTriPatch) - reinterpret_cast<unsigned char*>(pD3D9Dll.get());
    d3d9DeviceOffsets->Present = reinterpret_cast<unsigned char*>(pDevice->lpVtbl->Present) - reinterpret_cast<unsigned char*>(pD3D9Dll.get());
    d3d9DeviceOffsets->Reset = reinterpret_cast<unsigned char*>(pDevice->lpVtbl->Reset) - reinterpret_cast<unsigned char*>(pD3D9Dll.get());
}