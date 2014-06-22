#include "Hooks.h"

#include "InputHooks.h"

#include <Rpc.h>
#include <RpcNdr.h>
#include <RpcNsi.h>
#include <Unknwn.h>
#include <GdiPlus.h>

SharedData g_sharedData = {};
ULONG_PTR gdiplusToken;

USE_HOOK_MODULE(d3d9);

bool InitHooks()
{
    unique_sharedmem_ptr sharedMemPtr = OpenSharedMemory(FILE_MAPPING_OBJECT_NAME, sizeof(SharedData));
    if (!sharedMemPtr)
    {
        return false;
    }

    g_sharedData = *reinterpret_cast<SharedData*>(sharedMemPtr.get());

    if (MH_Initialize() != MH_OK)
    {
        return false;
    }

    ENABLE_HOOK_MODULE(d3d9);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    return true;
}

void RevertHooks()
{
    Gdiplus::GdiplusShutdown(gdiplusToken);

    DISABLE_HOOK_MODULE(d3d9);
    g_inputHooks.RevertHooks();
    MH_Uninitialize();
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

void SaveTexture(const wchar_t* name, size_t width, size_t height, void* pData, int pitch)
{
    wchar_t filename[260];
    wsprintf(filename, L"%s\\%s", g_sharedData.saveFolder, name);
    Gdiplus::Bitmap bitmap(width, height, pitch, PixelFormat32bppARGB, reinterpret_cast<unsigned char*>(pData));
    CLSID   encoderClsid;
    GetEncoderClsid(L"image/png", &encoderClsid);
    bitmap.Save(filename, &encoderClsid);
}

bool EnableModule(HookModule& hookModule)
{
    HMODULE hModule = GetModuleHandleA(hookModule.name);
    if (!hModule)
    {
        return true;
    }

    for (auto it = hookModule.hooks.begin(), it_end = hookModule.hooks.end(); it != it_end; ++it)
    {
        if ((*it)->CreateHook(hModule) == false)
        {
            return false;
        }
    }

    return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}

void DisableModule(HookModule& hookModule)
{
    for (auto it = hookModule.hooks.begin(), it_end = hookModule.hooks.end(); it != it_end; ++it)
    {
        (*it)->DisableHook();
    }

    MH_DisableHook(MH_ALL_HOOKS);

    for (auto it = hookModule.hooks.begin(), it_end = hookModule.hooks.end(); it != it_end; ++it)
    {
        (*it)->RemoveHook();
    }
}
