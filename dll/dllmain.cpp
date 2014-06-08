#include "Hooks.h"

#include "common/Platform.h"

DWORD WINAPI SelfDestruct(LPVOID lpThreadParameter)
{
    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, SHARED_MUTEX_NAME);
    WaitForSingleObject(hMutex, INFINITE);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(lpThreadParameter), 0);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        if (!InitHooks())
        {
            return FALSE;
        }
        CreateThread(NULL, 0, &SelfDestruct, hinstDLL, 0, NULL);
        return TRUE;
    }
    else if(fdwReason == DLL_PROCESS_DETACH)
    {
        RevertHooks();
    }

    return TRUE;
}