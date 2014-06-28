#include "Hooks.h"

#include "InputHooks.h"

#include <cassert>
#include <mutex>

#define CINTERFACE
#define COBJMACROS
#include <d3d9.h>

#define MAX_TEXTURE_COUNT 8

DECLARE_HOOK_MODULE(d3d9);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_Present, IDirect3DDevice9Present, g_sharedData.d3d9DeviceOffsets.Present, HRESULT, IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DIP, IDirect3DDevice9DIP, g_sharedData.d3d9DeviceOffsets.DrawIndexedPrimitive, HRESULT, IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DIPUP, IDirect3DDevice9DIPUP, g_sharedData.d3d9DeviceOffsets.DrawIndexedPrimitiveUP, HRESULT, IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT, UINT, CONST void*, D3DFORMAT, CONST void*, UINT);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DP, IDirect3DDevice9DP, g_sharedData.d3d9DeviceOffsets.DrawPrimitive, HRESULT, IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DPUP, IDirect3DDevice9DPUP, g_sharedData.d3d9DeviceOffsets.DrawPrimitiveUP, HRESULT, IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, CONST void*, UINT);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DrawRectPatch, IDirect3DDevice9DrawRectPatch, g_sharedData.d3d9DeviceOffsets.DrawRectPatch, HRESULT, IDirect3DDevice9*, UINT, CONST float*, CONST D3DRECTPATCH_INFO*);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DrawTriPatch, IDirect3DDevice9DrawTriPatch, g_sharedData.d3d9DeviceOffsets.DrawTriPatch, HRESULT, IDirect3DDevice9*, UINT, CONST float*, CONST D3DTRIPATCH_INFO*);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_Reset, IDirect3DDevice9Reset, g_sharedData.d3d9DeviceOffsets.Reset, HRESULT, IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

template<typename ComType>
void CComSafeRelease(ComType* comObject)
{
    if (comObject)
    {
        IUnknown_Release(comObject);
    }
}

struct D3D9DeviceInfo
{
    IDirect3DDevice9*   device;
    bool                isExDevice;
    D3DCAPS9            caps;
    D3DDEVICE_CREATION_PARAMETERS creationParams;
    D3DSURFACE_DESC backBufferDesc;
    IDirect3DStateBlock9* stateBlock;
    IDirect3DTexture9* overlayTexture;
};
D3D9DeviceInfo g_deviceInfo = {};

struct D3D9CaptureInfo
{
    std::mutex lock;
    bool isActive;
    size_t frameNumber;
    size_t drawCallNumber;
};
D3D9CaptureInfo g_d3d9CaptureInfo = {};

const float overlayQuad[] =
{
     0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
     0.0f, 64.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    64.0f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    64.0f, 64.0f, 0.0f, 1.0f, 1.0f, 1.0f,
};

TextureFormat ConvertD3D9TextureFormat(D3DFORMAT d3dFormat)
{
    switch (d3dFormat)
    {
    case D3DFMT_A8R8G8B8:
        return TextureFormat_ARGB;
    case D3DFMT_X8R8G8B8:
        return TextureFormat_XRGB;
    }

    log(L"Unimplemented texture format: %d", d3dFormat);
    return TextureFormat_Unknown;
}

void CreateOverlayTexture()
{
    if (g_deviceInfo.isExDevice)
    {
        IDirect3DTexture9* sysmemTexture = NULL;
        IDirect3DDevice9_CreateTexture(g_deviceInfo.device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sysmemTexture, NULL);
        D3DLOCKED_RECT lockedRect = {};
        IDirect3DTexture9_LockRect(sysmemTexture, 0, &lockedRect, NULL, 0);
        memset(lockedRect.pBits, 0xFF, 64 * 64 * 4);
        IDirect3DTexture9_UnlockRect(sysmemTexture, 0);
        IDirect3DDevice9_CreateTexture(g_deviceInfo.device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_deviceInfo.overlayTexture, NULL);

        IDirect3DBaseTexture9* pBaseSource = NULL;
        IDirect3DBaseTexture9* pBaseDest = NULL;
        IUnknown_QueryInterface(sysmemTexture, __uuidof(IDirect3DBaseTexture9), reinterpret_cast<void**>(&pBaseSource));
        IUnknown_QueryInterface(g_deviceInfo.overlayTexture, __uuidof(IDirect3DBaseTexture9), reinterpret_cast<void**>(&pBaseDest));
        IDirect3DDevice9_UpdateTexture(g_deviceInfo.device, pBaseSource, pBaseDest);
        CComSafeRelease(pBaseDest);
        CComSafeRelease(pBaseSource);
        CComSafeRelease(sysmemTexture);
    }
    else
    {
        IDirect3DDevice9_CreateTexture(g_deviceInfo.device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &g_deviceInfo.overlayTexture, NULL);
        D3DLOCKED_RECT lockedRect = {};
        IDirect3DTexture9_LockRect(g_deviceInfo.overlayTexture, 0, &lockedRect, NULL, 0);
        memset(lockedRect.pBits, 0xFF, 64 * 64 * 4);
        IDirect3DTexture9_UnlockRect(g_deviceInfo.overlayTexture, 0);
    }
}

void UpdateDeviceInfo(IDirect3DDevice9* device)
{
    assert(device);

    CComSafeRelease(g_deviceInfo.overlayTexture);
    CComSafeRelease(g_deviceInfo.stateBlock);

    IDirect3DDevice9Ex* pDeviceEx = NULL;
    IUnknown_QueryInterface(device, __uuidof(IDirect3DDevice9Ex), reinterpret_cast<void**>(&pDeviceEx));
    g_deviceInfo.isExDevice = pDeviceEx != NULL;
    CComSafeRelease(pDeviceEx);
   
    IDirect3DDevice9_GetDeviceCaps(device, &g_deviceInfo.caps);
    IDirect3DDevice9_GetCreationParameters(device, &g_deviceInfo.creationParams);
    IDirect3DSurface9* pBackBuffer = NULL;
    IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    IDirect3DSurface9_GetDesc(pBackBuffer, &g_deviceInfo.backBufferDesc);
    CComSafeRelease(pBackBuffer);
    g_inputHooks.HookWindow(g_deviceInfo.creationParams.hFocusWindow);
    IDirect3DDevice9_CreateStateBlock(device, D3DSBT_ALL, &g_deviceInfo.stateBlock);
    g_deviceInfo.device = device;
    CreateOverlayTexture();
}

void D3D9CaptureTextures(IDirect3DDevice9* pThis)
{
    IDirect3DBaseTexture9* srcTextures[MAX_TEXTURE_COUNT] = {};
    
    for (DWORD i = 0; i < MAX_TEXTURE_COUNT; ++i)
    {
        IDirect3DDevice9_GetTexture(pThis, i, &srcTextures[i]);
        if (srcTextures[i] == nullptr)
        {
            break;
        }

        IDirect3DTexture9* texture = nullptr;
        IDirect3DBaseTexture9_QueryInterface(srcTextures[i], __uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&texture));
        if (texture == nullptr)
        {
            continue;
        }

        wchar_t textureName[MAX_PATH];
        wsprintf(textureName, L"texture_%d_%d", g_d3d9CaptureInfo.drawCallNumber, i);
        D3DSURFACE_DESC desc = {};
        IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        D3DLOCKED_RECT lockedRect = {};
        IDirect3DTexture9_LockRect(texture, 0, &lockedRect, NULL, D3DLOCK_READONLY);
        if (lockedRect.pBits)
        {
            SaveTexture(textureName, desc.Width, desc.Height, ConvertD3D9TextureFormat(desc.Format), lockedRect.pBits, lockedRect.Pitch);
            IDirect3DTexture9_UnlockRect(texture, 0);
        }
        else
        {
            log(L"Failed to lock texture");
        }

        CComSafeRelease(texture);
    }

    for (DWORD i = 0; i < MAX_TEXTURE_COUNT; ++i)
    {
        CComSafeRelease(srcTextures[i]);
    }
}

HRESULT __stdcall Hooked_IDirect3DDevice9Present(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    if (pThis != g_deviceInfo.device)
    {
        UpdateDeviceInfo(pThis);
    }

    IDirect3DStateBlock9_Capture(g_deviceInfo.stateBlock);

    IDirect3DDevice9_BeginScene(pThis);

    IDirect3DDevice9_SetDepthStencilSurface(pThis, NULL);
    IDirect3DSurface9* pBackBuffer = NULL;
    IDirect3DDevice9_GetBackBuffer(pThis, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    IDirect3DDevice9_SetRenderTarget(pThis, 0, pBackBuffer);
    IDirect3DDevice9_SetVertexShader(pThis, NULL);
    IDirect3DDevice9_SetFVF(pThis, D3DFVF_XYZRHW | D3DFVF_TEX0);
    IDirect3DDevice9_SetPixelShader(pThis, NULL);
    RECT scissor = { 0, 0, g_deviceInfo.backBufferDesc.Width, g_deviceInfo.backBufferDesc.Height };
    IDirect3DDevice9_SetScissorRect(pThis, &scissor);

    IDirect3DBaseTexture9* baseTexture = NULL;
    IUnknown_QueryInterface(g_deviceInfo.overlayTexture, __uuidof(IDirect3DBaseTexture9), reinterpret_cast<void**>(&baseTexture));
    IDirect3DDevice9_SetTexture(pThis, 0, baseTexture);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_ALPHABLENDENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_BLENDOP, D3DBLENDOP_ADD);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_ZWRITEENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_BLENDFACTOR, 0xFFFFFFFF);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_STENCILENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_AMBIENT, 0);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_ZENABLE, D3DZB_FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_FILLMODE, D3DFILL_SOLID);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_ALPHATESTENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_DITHERENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_FOGENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_SPECULARENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_CLIPPING, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_CLIPPLANEENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_COLORWRITEENABLE, 0x0000000F);
    IDirect3DDevice9_SetRenderState(pThis, D3DRS_SRGBWRITEENABLE, FALSE);

    IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
    IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
    IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_ADDRESSW, D3DTADDRESS_BORDER);
    IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_SRGBTEXTURE, FALSE);

    IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

    for (DWORD stage = 1; stage < D3DDP_MAXTEXCOORD; ++stage)
    {
        IDirect3DDevice9_SetTextureStageState(pThis, stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
        IDirect3DDevice9_SetTextureStageState(pThis, stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }

    hIDirect3DDevice9_DPUP.m_real(pThis, D3DPT_TRIANGLESTRIP, 2, overlayQuad, sizeof(float) * 6);
    
    CComSafeRelease(baseTexture);
    CComSafeRelease(pBackBuffer);

    IDirect3DDevice9_EndScene(pThis);

    IDirect3DStateBlock9_Apply(g_deviceInfo.stateBlock);

    {
        std::lock_guard<std::mutex> lock(g_d3d9CaptureInfo.lock);
        if (g_d3d9CaptureInfo.isActive)
        {
            log(L"IDirect3DDevice9::Present");
            g_d3d9CaptureInfo.isActive = false;
            g_d3d9CaptureInfo.frameNumber += 1;
            g_inputHooks.ResetCapture();
        }
    }

    HRESULT hr = hIDirect3DDevice9_Present.m_real(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

    if (g_inputHooks.IsCaptureActive())
    {
        std::lock_guard<std::mutex> lock(g_d3d9CaptureInfo.lock);
        g_d3d9CaptureInfo.isActive = true;
        g_d3d9CaptureInfo.drawCallNumber = 0;
    }
    return hr;
}

HRESULT __stdcall Hooked_IDirect3DDevice9DIP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
    {
        std::lock_guard<std::mutex> lock(g_d3d9CaptureInfo.lock);
        if (g_d3d9CaptureInfo.isActive)
        {
            log(L"IDirect3DDevice9::DrawIndexedPrimitive");
            D3D9CaptureTextures(pThis);
            g_d3d9CaptureInfo.drawCallNumber += 1;
        }
    }
    return hIDirect3DDevice9_DIP.m_real(pThis, Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DIPUP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    {
        std::lock_guard<std::mutex> lock(g_d3d9CaptureInfo.lock);
        if (g_d3d9CaptureInfo.isActive)
        {
            log(L"IDirect3DDevice9::DrawIndexedPrimitiveUP");
        }
    }
    return hIDirect3DDevice9_DIPUP.m_real(pThis, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE Type, UINT MinIndex, UINT NumVertices)
{
    {
        std::lock_guard<std::mutex> lock(g_d3d9CaptureInfo.lock);
        if (g_d3d9CaptureInfo.isActive)
        {
            log(L"IDirect3DDevice9::DrawPrimitive");
        }
    }
    return hIDirect3DDevice9_DP.m_real(pThis, Type, MinIndex, NumVertices);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DPUP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    {
        std::lock_guard<std::mutex> lock(g_d3d9CaptureInfo.lock);
        if (g_d3d9CaptureInfo.isActive)
        {
            log(L"IDirect3DDevice9::DrawPrimitiveUP");
        }
    }
    return hIDirect3DDevice9_DPUP.m_real(pThis, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DrawRectPatch(IDirect3DDevice9* pThis, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
    {
        std::lock_guard<std::mutex> lock(g_d3d9CaptureInfo.lock);
        if (g_d3d9CaptureInfo.isActive)
        {
            log(L"IDirect3DDevice9::DrawRectPatch");
        }
    }
    return hIDirect3DDevice9_DrawRectPatch.m_real(pThis, Handle, pNumSegs, pRectPatchInfo);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DrawTriPatch(IDirect3DDevice9* pThis, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
    {
        std::lock_guard<std::mutex> lock(g_d3d9CaptureInfo.lock);
        if (g_d3d9CaptureInfo.isActive)
        {
            log(L"IDirect3DDevice9::DrawTriPatch");
        }
    }
    return hIDirect3DDevice9_DrawTriPatch.m_real(pThis, Handle, pNumSegs, pTriPatchInfo);
}

HRESULT __stdcall Hooked_IDirect3DDevice9Reset(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    log(L"IDirect3DDevice9::Reset");
    CComSafeRelease(g_deviceInfo.stateBlock);
    HRESULT hr = hIDirect3DDevice9_Reset.m_real(pThis, pPresentationParameters);
    if (SUCCEEDED(hr))
    {
        UpdateDeviceInfo(pThis);
    }
    return hr;
}
