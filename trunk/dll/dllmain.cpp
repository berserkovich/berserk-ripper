
#include "RipperDX9.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return InitHooks() ? TRUE : FALSE;
    }

    return TRUE;
}