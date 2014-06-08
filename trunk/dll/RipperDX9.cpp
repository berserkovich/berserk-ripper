#include "RipperDX9.h"

#include "Hooks.h"

#include "MinHook.h"

#include <cassert>

#define CINTERFACE
#include <d3d9.h>

template<typename ComType>
void CComSafeRelease(ComType* comObject)
{
    if (comObject)
    {
        comObject->lpVtbl->Release(comObject);
    }
}

struct D3D9DeviceInfo
{
    IDirect3DDevice9*   device;
    D3DCAPS9            caps;
    D3DDEVICE_CREATION_PARAMETERS creationParams;
    D3DSURFACE_DESC backBufferDesc;
};
D3D9DeviceInfo g_deviceInfo = {};

struct D3D9DeviceState
{
    D3DMATRIX world;
    D3DMATRIX view;
    D3DMATRIX projection;
    D3DMATRIX texture0;
    DWORD fvf;
    D3DVIEWPORT9 viewport;
    RECT scissor;
    DWORD renderState[256];
    DWORD samplerState[16];

    IDirect3DSurface9* pZStencilSurface;
    IDirect3DSurface9* pRenderTargets[D3D_MAX_SIMULTANEOUS_RENDERTARGETS];
    IDirect3DVertexShader9* pVertexShader;
    IDirect3DPixelShader9* pPixelShader;
    IDirect3DVertexDeclaration9* pVertexDeclaration;
    IDirect3DVertexBuffer9* pVertexBuffer;
    UINT vertexBufferOffset;
    UINT vertexBufferStride;
    IDirect3DIndexBuffer9* pIndexBuffer;
};

void UpdateDeviceInfo(IDirect3DDevice9* device)
{
    device->lpVtbl->GetDeviceCaps(device, &g_deviceInfo.caps);
    device->lpVtbl->GetCreationParameters(device, &g_deviceInfo.creationParams);
    IDirect3DSurface9* pBackBuffer = NULL;
    device->lpVtbl->GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    pBackBuffer->lpVtbl->GetDesc(pBackBuffer, &g_deviceInfo.backBufferDesc);
    CComSafeRelease(pBackBuffer);
    g_deviceInfo.device = device;
}

void SaveState(IDirect3DDevice9* device, D3D9DeviceState* state)
{
    assert(device);
    assert(state);

    device->lpVtbl->GetDepthStencilSurface(device, &state->pZStencilSurface);
    for (DWORD i = 0; i < g_deviceInfo.caps.NumSimultaneousRTs; ++i)
    {
        device->lpVtbl->GetRenderTarget(device, i, &state->pRenderTargets[i]);
    }
    device->lpVtbl->GetVertexShader(device, &state->pVertexShader);
    if (state->pVertexShader == NULL)
    {
        device->lpVtbl->GetFVF(device, &state->fvf);
    }
    else
    {
        device->lpVtbl->GetVertexDeclaration(device, &state->pVertexDeclaration);
    }
    device->lpVtbl->GetPixelShader(device, &state->pPixelShader);
    device->lpVtbl->GetTransform(device, D3DTS_WORLD, &state->world);
    device->lpVtbl->GetTransform(device, D3DTS_VIEW, &state->view);
    device->lpVtbl->GetTransform(device, D3DTS_PROJECTION, &state->projection);
    device->lpVtbl->GetTransform(device, D3DTS_TEXTURE0, &state->texture0);
    device->lpVtbl->GetViewport(device, &state->viewport);
    device->lpVtbl->GetScissorRect(device, &state->scissor);
    device->lpVtbl->GetStreamSource(device, 0, &state->pVertexBuffer, &state->vertexBufferOffset, &state->vertexBufferStride);
    device->lpVtbl->GetIndices(device, &state->pIndexBuffer);
    for (DWORD i = D3DRS_ZENABLE; i < D3DRS_BLENDOPALPHA; ++i)
    {
        device->lpVtbl->GetRenderState(device, static_cast<D3DRENDERSTATETYPE>(i), &state->renderState[i]);
    }
    for (DWORD i = D3DSAMP_ADDRESSU; i < D3DSAMP_DMAPOFFSET; ++i)
    {
        device->lpVtbl->GetSamplerState(device, 0, static_cast<D3DSAMPLERSTATETYPE>(i), &state->samplerState[i]);
    }
}

void LoadState(IDirect3DDevice9* device, const D3D9DeviceState& state)
{
    assert(device);

    device->lpVtbl->SetDepthStencilSurface(device, state.pZStencilSurface);
    CComSafeRelease(state.pZStencilSurface);
    for (DWORD i = 0; i < g_deviceInfo.caps.NumSimultaneousRTs; ++i)
    {
        device->lpVtbl->SetRenderTarget(device, i, state.pRenderTargets[i]);
        CComSafeRelease(state.pRenderTargets[i]);
    }
    device->lpVtbl->SetVertexShader(device, state.pVertexShader);
    if (state.pVertexShader == NULL)
    {
        device->lpVtbl->SetFVF(device, state.fvf);
    }
    else
    {
        device->lpVtbl->SetVertexDeclaration(device, state.pVertexDeclaration);
    }
    device->lpVtbl->SetPixelShader(device, state.pPixelShader);
    device->lpVtbl->SetTransform(device, D3DTS_WORLD, &state.world);
    device->lpVtbl->SetTransform(device, D3DTS_VIEW, &state.view);
    device->lpVtbl->SetTransform(device, D3DTS_PROJECTION, &state.projection);
    device->lpVtbl->SetTransform(device, D3DTS_TEXTURE0, &state.texture0);
    device->lpVtbl->SetViewport(device, &state.viewport);
    device->lpVtbl->SetScissorRect(device, &state.scissor);
    device->lpVtbl->SetStreamSource(device, 0, state.pVertexBuffer, state.vertexBufferOffset, state.vertexBufferStride);
    CComSafeRelease(state.pVertexBuffer);
    device->lpVtbl->SetIndices(device, state.pIndexBuffer);
    CComSafeRelease(state.pIndexBuffer);
    for (DWORD i = D3DRS_ZENABLE; i < D3DRS_BLENDOPALPHA; ++i)
    {
        device->lpVtbl->SetRenderState(device, static_cast<D3DRENDERSTATETYPE>(i), state.renderState[i]);
    }
    for (DWORD i = D3DSAMP_ADDRESSU; i < D3DSAMP_DMAPOFFSET; ++i)
    {
        device->lpVtbl->SetSamplerState(device, 0, static_cast<D3DSAMPLERSTATETYPE>(i), state.samplerState[i]);
    }
}

typedef HRESULT(STDMETHODCALLTYPE* IDirect3DDevice9_PresentPtr)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
IDirect3DDevice9_PresentPtr Target_IDirect3DDevice9_Present = NULL;
IDirect3DDevice9_PresentPtr Real_IDirect3DDevice9_Present = NULL;

HRESULT Hooked_IDirect3DDevice9_Present(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    if (pThis != g_deviceInfo.device)
    {
        UpdateDeviceInfo(pThis);
    }

    pThis->lpVtbl->BeginScene(pThis);

    D3D9DeviceState savedState = {};
    SaveState(pThis, &savedState);

    pThis->lpVtbl->SetDepthStencilSurface(pThis, NULL);
    IDirect3DSurface9* pBackBuffer = NULL;
    pThis->lpVtbl->GetBackBuffer(pThis, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    pThis->lpVtbl->SetRenderTarget(pThis, 0, pBackBuffer);
    pThis->lpVtbl->SetVertexShader(pThis, NULL);
    pThis->lpVtbl->SetFVF(pThis, D3DFVF_XYZRHW);
    pThis->lpVtbl->SetPixelShader(pThis, NULL);
    D3DMATRIX indentityMatrix = { 1.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 1.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 0.0f, 1.0f };
    pThis->lpVtbl->SetTransform(pThis, D3DTS_WORLD, &indentityMatrix);
    pThis->lpVtbl->SetTransform(pThis, D3DTS_VIEW, &indentityMatrix);
    pThis->lpVtbl->SetTransform(pThis, D3DTS_PROJECTION, &indentityMatrix);
    pThis->lpVtbl->SetTransform(pThis, D3DTS_TEXTURE0, &indentityMatrix);
    D3DVIEWPORT9 viewport = {
        0, 0,
        g_deviceInfo.backBufferDesc.Width,
        g_deviceInfo.backBufferDesc.Height,
        0.0f, 1.0f
    };
    pThis->lpVtbl->SetViewport(pThis, &viewport);
    RECT scissor = { 0, 0, g_deviceInfo.backBufferDesc.Width, g_deviceInfo.backBufferDesc.Height };
    pThis->lpVtbl->SetScissorRect(pThis, &scissor);
    
    LoadState(pThis, savedState);
    CComSafeRelease(pBackBuffer);

    pThis->lpVtbl->EndScene(pThis);

    return Real_IDirect3DDevice9_Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

bool InitD3D9Hooks(HMODULE hModule)
{
    Target_IDirect3DDevice9_Present = reinterpret_cast<IDirect3DDevice9_PresentPtr>(reinterpret_cast<unsigned char*>(hModule)+g_sharedData.d3d9DeviceOffsets.Present);
    if (MH_CreateHook(Target_IDirect3DDevice9_Present, &Hooked_IDirect3DDevice9_Present, reinterpret_cast<void**>(&Real_IDirect3DDevice9_Present)) != MH_OK
        || MH_QueueEnableHook(Target_IDirect3DDevice9_Present) != MH_OK)
    {
        return false;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        return false;
    }

    return true;
}

void RevertD3D9Hooks()
{
    MH_QueueDisableHook(Target_IDirect3DDevice9_Present);
    MH_DisableHook(MH_ALL_HOOKS);

    MH_RemoveHook(Target_IDirect3DDevice9_Present);
}