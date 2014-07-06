#include "Hooks.h"

#include <cassert>

#define CINTERFACE
#define COBJMACROS
#include <d3d9.h>

#include "D3D9Context.h"

void D3D9Init();
void D3D9Cleanup();

DECLARE_HOOK_MODULE(d3d9, &D3D9Init, &D3D9Cleanup);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_Present, IDirect3DDevice9Present, g_sharedData.d3d9DeviceOffsets.Present, HRESULT, IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DIP, IDirect3DDevice9DIP, g_sharedData.d3d9DeviceOffsets.DrawIndexedPrimitive, HRESULT, IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DIPUP, IDirect3DDevice9DIPUP, g_sharedData.d3d9DeviceOffsets.DrawIndexedPrimitiveUP, HRESULT, IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT, UINT, CONST void*, D3DFORMAT, CONST void*, UINT);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DP, IDirect3DDevice9DP, g_sharedData.d3d9DeviceOffsets.DrawPrimitive, HRESULT, IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DPUP, IDirect3DDevice9DPUP, g_sharedData.d3d9DeviceOffsets.DrawPrimitiveUP, HRESULT, IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, CONST void*, UINT);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DrawRectPatch, IDirect3DDevice9DrawRectPatch, g_sharedData.d3d9DeviceOffsets.DrawRectPatch, HRESULT, IDirect3DDevice9*, UINT, CONST float*, CONST D3DRECTPATCH_INFO*);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_DrawTriPatch, IDirect3DDevice9DrawTriPatch, g_sharedData.d3d9DeviceOffsets.DrawTriPatch, HRESULT, IDirect3DDevice9*, UINT, CONST float*, CONST D3DTRIPATCH_INFO*);
DECLARE_HOOK(d3d9, hIDirect3DDevice9_Reset, IDirect3DDevice9Reset, g_sharedData.d3d9DeviceOffsets.Reset, HRESULT, IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

D3D9Context* g_d3d9Context = NULL;

HRESULT __stdcall Hooked_IDirect3DDevice9Present(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{

    //IDirect3DStateBlock9_Capture(g_deviceInfo.stateBlock);

    //IDirect3DDevice9_BeginScene(pThis);

    //IDirect3DDevice9_SetDepthStencilSurface(pThis, NULL);
    //IDirect3DSurface9* pBackBuffer = NULL;
    //IDirect3DDevice9_GetBackBuffer(pThis, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    //IDirect3DDevice9_SetRenderTarget(pThis, 0, pBackBuffer);
    //IDirect3DDevice9_SetVertexShader(pThis, NULL);
    //IDirect3DDevice9_SetFVF(pThis, D3DFVF_XYZRHW | D3DFVF_TEX0);
    //IDirect3DDevice9_SetPixelShader(pThis, NULL);
    //RECT scissor = { 0, 0, g_deviceInfo.backBufferDesc.Width, g_deviceInfo.backBufferDesc.Height };
    //IDirect3DDevice9_SetScissorRect(pThis, &scissor);

    //IDirect3DBaseTexture9* baseTexture = NULL;
    //IUnknown_QueryInterface(g_deviceInfo.overlayTexture, __uuidof(IDirect3DBaseTexture9), reinterpret_cast<void**>(&baseTexture));
    //IDirect3DDevice9_SetTexture(pThis, 0, baseTexture);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_LIGHTING, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_CULLMODE, D3DCULL_NONE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_ALPHABLENDENABLE, TRUE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_BLENDOP, D3DBLENDOP_ADD);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_ZWRITEENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_BLENDFACTOR, 0xFFFFFFFF);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_STENCILENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_AMBIENT, 0);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_ZENABLE, D3DZB_FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_FILLMODE, D3DFILL_SOLID);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_ALPHATESTENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_DITHERENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_FOGENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_SPECULARENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_CLIPPING, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_CLIPPLANEENABLE, FALSE);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_COLORWRITEENABLE, 0x0000000F);
    //IDirect3DDevice9_SetRenderState(pThis, D3DRS_SRGBWRITEENABLE, FALSE);

    //IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    //IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    //IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    //IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
    //IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
    //IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_ADDRESSW, D3DTADDRESS_BORDER);
    //IDirect3DDevice9_SetSamplerState(pThis, 0, D3DSAMP_SRGBTEXTURE, FALSE);

    //IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    //IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    //IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    //IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    //IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    //IDirect3DDevice9_SetTextureStageState(pThis, 0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

    //for (DWORD stage = 1; stage < D3DDP_MAXTEXCOORD; ++stage)
    //{
    //    IDirect3DDevice9_SetTextureStageState(pThis, stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
    //    IDirect3DDevice9_SetTextureStageState(pThis, stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    //}

    //hIDirect3DDevice9_DPUP.m_real(pThis, D3DPT_TRIANGLESTRIP, 2, overlayQuad, sizeof(float) * 6);
    //
    //CComSafeRelease(baseTexture);
    //CComSafeRelease(pBackBuffer);

    //IDirect3DDevice9_EndScene(pThis);

    //IDirect3DStateBlock9_Apply(g_deviceInfo.stateBlock);

    g_d3d9Context->OnPresent(pThis);

    HRESULT hr = hIDirect3DDevice9_Present.m_real(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

    g_d3d9Context->PostPresent();

    return hr;
}

HRESULT __stdcall Hooked_IDirect3DDevice9DIP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
    g_d3d9Context->OnDrawCall();
    return hIDirect3DDevice9_DIP.m_real(pThis, Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DIPUP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    g_d3d9Context->OnDrawCall();
    return hIDirect3DDevice9_DIPUP.m_real(pThis, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE Type, UINT MinIndex, UINT NumVertices)
{
    g_d3d9Context->OnDrawCall();
    return hIDirect3DDevice9_DP.m_real(pThis, Type, MinIndex, NumVertices);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DPUP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    g_d3d9Context->OnDrawCall();
    return hIDirect3DDevice9_DPUP.m_real(pThis, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DrawRectPatch(IDirect3DDevice9* pThis, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
    g_d3d9Context->OnDrawCall();
    return hIDirect3DDevice9_DrawRectPatch.m_real(pThis, Handle, pNumSegs, pRectPatchInfo);
}

HRESULT __stdcall Hooked_IDirect3DDevice9DrawTriPatch(IDirect3DDevice9* pThis, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
    g_d3d9Context->OnDrawCall();
    return hIDirect3DDevice9_DrawTriPatch.m_real(pThis, Handle, pNumSegs, pTriPatchInfo);
}

HRESULT __stdcall Hooked_IDirect3DDevice9Reset(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    LOG("IDirect3DDevice9::Reset");
    g_d3d9Context->CleanupDevice();
    HRESULT hr = hIDirect3DDevice9_Reset.m_real(pThis, pPresentationParameters);
    if (SUCCEEDED(hr))
    {
        g_d3d9Context->SetDevice(pThis);
    }
    return hr;
}

void D3D9Init()
{
    g_d3d9Context = new D3D9Context();
}

void D3D9Cleanup()
{
    g_d3d9Context->RequestCleanup();
    delete g_d3d9Context;
    g_d3d9Context = nullptr;
}
