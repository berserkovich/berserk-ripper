#include "Hooks.h"

#include "ImageCache.h"
#include "InputHooks.h"

SharedData g_sharedData = {};
ImageCache* g_imageCache = nullptr;

USE_HOOK_MODULE(d3d9);

bool InitHooks()
{
    unique_sharedmem_ptr sharedMemPtr = OpenSharedMemory(FILE_MAPPING_OBJECT_NAME, sizeof(SharedData));
    if (!sharedMemPtr)
    {
        return false;
    }

    g_sharedData = *reinterpret_cast<SharedData*>(sharedMemPtr.get());
    g_imageCache = new ImageCache(WideCharToUtf8(g_sharedData.saveFolder));

    if (MH_Initialize() != MH_OK)
    {
        return false;
    }


    ENABLE_HOOK_MODULE(d3d9);

    return true;
}

void RevertHooks()
{
    DISABLE_HOOK_MODULE(d3d9);
    g_inputHooks.RevertHooks();
    MH_Uninitialize();
    delete g_imageCache;
}

void SaveTexture(size_t width, size_t height, TextureFormat format, void* pData, int pitch)
{
    if (format == TextureFormat_Unknown)
    {
        return;
    }

    if (format == TextureFormat_ARGB
        || format == TextureFormat_XRGB)
    {
    }
    else if (format >= TextureFormat_DXT1
        && format <= TextureFormat_DXT5)
    {
        g_imageCache->Add(width, height, format, pData, pitch);
    }
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

    if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
    {
        if (hookModule.pfnInit)
        {
            hookModule.pfnInit();
        }
        return true;
    }

    return false;
}

void DisableModule(HookModule& hookModule)
{
    if (hookModule.pfnCleanup)
    {
        hookModule.pfnCleanup();
    }

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
