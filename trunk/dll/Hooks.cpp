#include "Hooks.h"

#include "common/Platform.h"
#include "RipperDX9.h"

#include "MinHook.h"

SharedData g_sharedData = {};

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

    HMODULE hD3D9 = GetModuleHandle(L"d3d9.dll");
    if (hD3D9)
    {
        InitD3D9Hooks(hD3D9);
    }

    return true;
}

void RevertHooks()
{
    RevertD3D9Hooks();
    MH_Uninitialize();
}