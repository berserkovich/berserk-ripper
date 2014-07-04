#pragma once

#include "common/Platform.h"

#include "D3DHelpers.h"

#include <mutex>

#include <d3d9.h>

class D3D9Context
{
public:
    D3D9Context();
    ~D3D9Context();

    void SetDevice(IDirect3DDevice9* device);
    void CleanupDevice();
    void RequestCleanup();

    void OnPresent(IDirect3DDevice9* device);
    void PostPresent();
    void OnDrawCall();

private:
    void SetDevice_(IDirect3DDevice9* device);
    void CleanupDevice_();
    void CreateOverlayTexture_();
    void CaptureTextures_();

private:
    std::mutex m_lock;

    com_ptr<IDirect3DDevice9> m_device;
    bool m_isExDevice;
    D3DCAPS9 m_caps;
    D3DDEVICE_CREATION_PARAMETERS m_creationParams;
    D3DSURFACE_DESC m_backBufferDesc;
    com_ptr<IDirect3DTexture9> m_overlayTexture;

    bool m_isCaptureActive;
    size_t m_captureFrameNumber;
    size_t m_captureDrawCallNumber;
    bool m_cleanup;

    HANDLE m_cleanupEvent;
};