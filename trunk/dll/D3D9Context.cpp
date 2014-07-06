#include "D3D9Context.h"

#include "common/Common.h"

#include "Hooks.h"
#include "InputHooks.h"

const float overlayQuad[] =
{
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 64.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    64.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
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
    case D3DFMT_DXT1:
        return TextureFormat_DXT1;
    case D3DFMT_DXT2:
        return TextureFormat_DXT2;
    case D3DFMT_DXT3:
        return TextureFormat_DXT3;
    case D3DFMT_DXT4:
        return TextureFormat_DXT4;
    case D3DFMT_DXT5:
        return TextureFormat_DXT5;
    }

    LOG("Unimplemented texture format: %d", d3dFormat);
    return TextureFormat_Unknown;
}

D3D9Context::D3D9Context()
    : m_device(nullptr)
    , m_isExDevice(false)
    , m_overlayTexture(nullptr)
    , m_isCaptureActive(false)
    , m_captureFrameNumber(0)
    , m_captureDrawCallNumber(0)
    , m_cleanup(false)
{
    m_cleanupEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

D3D9Context::~D3D9Context()
{
    CloseHandle(m_cleanupEvent);
}

void D3D9Context::SetDevice(IDirect3DDevice9* device)
{
    std::lock_guard<std::mutex> lock(m_lock);
    SetDevice_(device);
}

void D3D9Context::CleanupDevice()
{
    std::lock_guard<std::mutex> lock(m_lock);
    CleanupDevice_();
}

void D3D9Context::RequestCleanup()
{
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_cleanup = true;
    }
    WaitForSingleObject(m_cleanupEvent, INFINITE);
}

void D3D9Context::OnPresent(IDirect3DDevice9* device)
{
    std::lock_guard<std::mutex> lock(m_lock);

    if (device != m_device.get())
    {
        SetDevice_(device);
    }

    if (m_isCaptureActive)
    {
        LOG("IDirect3DDevice9::Present");
        m_isCaptureActive = false;
        m_captureFrameNumber += 1;
        g_inputHooks.ResetCapture();
    }
}

void D3D9Context::PostPresent()
{
    std::lock_guard<std::mutex> lock(m_lock);
    if (m_cleanup)
    {
        CleanupDevice_();
        SetEvent(m_cleanupEvent);
        return;
    }

    if (g_inputHooks.IsCaptureActive())
    {
        m_isCaptureActive = true;
        m_captureDrawCallNumber = 0;
    }
}

void D3D9Context::OnDrawCall()
{
    std::lock_guard<std::mutex> lock(m_lock);
    if (m_isCaptureActive)
    {
        CaptureTextures_();
        m_captureDrawCallNumber += 1;
    }
}

void D3D9Context::SetDevice_(IDirect3DDevice9* device)
{
    CleanupDevice_();

    m_isExDevice = false;

    if (!device)
    {
        LOG("device is null");
        return;
    }

    {
        com_ptr<IDirect3DDevice9Ex> pDeviceEx;
        device->QueryInterface(__uuidof(IDirect3DDevice9Ex), reinterpret_cast<void**>(&pDeviceEx));
        if (pDeviceEx)
        {
            m_isExDevice = true;
        }
    }

    D3DCALL(device->GetDeviceCaps(&m_caps));
    D3DCALL(device->GetCreationParameters(&m_creationParams));

    com_ptr<IDirect3DSurface9> pBackBuffer;
    D3DCALL(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer));

    pBackBuffer->GetDesc(&m_backBufferDesc);
    pBackBuffer->Release();
    g_inputHooks.HookWindow(m_creationParams.hFocusWindow);
    m_device.reset(device);
    CreateOverlayTexture_();
}

void D3D9Context::CleanupDevice_()
{
    m_overlayTexture.reset();
    m_device.reset();
}

void D3D9Context::CreateOverlayTexture_()
{
    if (m_isExDevice)
    {
        com_ptr<IDirect3DTexture9> sysmemTexture = nullptr;
        D3DCALL(m_device->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sysmemTexture, NULL));
        D3DLOCKED_RECT lockedRect = {};
        D3DCALL(sysmemTexture->LockRect(0, &lockedRect, NULL, 0));
        memset(lockedRect.pBits, 0xFF, 64 * 64 * 4);
        D3DCALL(sysmemTexture->UnlockRect(0));

        D3DCALL(m_device->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_overlayTexture, NULL));
        D3DCALL(m_device->UpdateTexture(sysmemTexture.get(), m_overlayTexture.get()));
    }
    else
    {
        D3DCALL(m_device->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_overlayTexture, NULL));
        D3DLOCKED_RECT lockedRect = {};
        D3DCALL(m_overlayTexture->LockRect(0, &lockedRect, NULL, 0));
        memset(lockedRect.pBits, 0xFF, 64 * 64 * 4);
        D3DCALL(m_overlayTexture->UnlockRect(0));
    }
}

void D3D9Context::CaptureTextures_()
{
    com_ptr<IDirect3DBaseTexture9> srcTextures[MAX_TEXTURE_COUNT] = {};

    for (DWORD i = 0; i < MAX_TEXTURE_COUNT; ++i)
    {
        m_device->GetTexture(i, &srcTextures[i]);
        if (!srcTextures[i])
        {
            return;
        }

        com_ptr<IDirect3DTexture9> texture = nullptr;
        srcTextures[i]->QueryInterface(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&texture));
        if (!texture)
        {
            continue;
        }

        D3DSURFACE_DESC desc = {};
        texture->GetLevelDesc(0, &desc);
        D3DLOCKED_RECT lockedRect = {};
        texture->LockRect(0, &lockedRect, NULL, D3DLOCK_READONLY);
        if (lockedRect.pBits)
        {
            SaveTexture(desc.Width, desc.Height, ConvertD3D9TextureFormat(desc.Format), lockedRect.pBits, lockedRect.Pitch);
            texture->UnlockRect(0);
        }
        else
        {
            LOG("Failed to lock texture");
        }
    }
}

