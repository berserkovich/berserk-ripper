#include "D3D9Hooks.h"

#include "Hooks.h"
#include "InputHooks.h"

#include "MinHook.h"

#include <cassert>

#define CINTERFACE
#define COBJMACROS
#include <d3d9.h>

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

const float overlayQuad[] =
{
     0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
     0.0f, 64.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    64.0f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    64.0f, 64.0f, 0.0f, 1.0f, 1.0f, 1.0f,
};

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

typedef HRESULT(STDMETHODCALLTYPE* IDirect3DDevice9_PresentPtr)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
IDirect3DDevice9_PresentPtr Target_IDirect3DDevice9_Present = NULL;
IDirect3DDevice9_PresentPtr Real_IDirect3DDevice9_Present = NULL;

HRESULT Hooked_IDirect3DDevice9_Present(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
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

    IDirect3DDevice9_DrawPrimitiveUP(pThis, D3DPT_TRIANGLESTRIP, 2, overlayQuad, sizeof(float) * 6);
    
    CComSafeRelease(baseTexture);
    CComSafeRelease(pBackBuffer);

    IDirect3DDevice9_EndScene(pThis);

    IDirect3DStateBlock9_Apply(g_deviceInfo.stateBlock);

    return Real_IDirect3DDevice9_Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

bool InitD3D9Hooks(HMODULE hModule)
{
   // MessageBox(NULL, L"attach", L"ripper", 0);

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
    if (Target_IDirect3DDevice9_Present == NULL)
    {
        return;
    }

    MH_QueueDisableHook(Target_IDirect3DDevice9_Present);
    MH_DisableHook(MH_ALL_HOOKS);

    MH_RemoveHook(Target_IDirect3DDevice9_Present);
}