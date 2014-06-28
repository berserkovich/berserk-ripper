#include "Hooks.h"

#include "InputHooks.h"

SharedData g_sharedData = {};

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

    return true;
}

void RevertHooks()
{
    DISABLE_HOOK_MODULE(d3d9);
    g_inputHooks.RevertHooks();
    MH_Uninitialize();
}

void SaveTexture(const wchar_t* name, size_t width, size_t height, TextureFormat format, void* pData, int pitch)
{
    if (format == TextureFormat_Unknown)
    {
        return;
    }

    wchar_t filename[260];
    if (format == TextureFormat_ARGB
        || format == TextureFormat_XRGB)
    {
        wsprintf(filename, L"%s\\%s.tga", g_sharedData.saveFolder, name);
        WriteTGA(filename, width, height, format, pData, pitch);
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
