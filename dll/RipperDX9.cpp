#include "RipperDX9.h"

#include "MinHook.h"

bool InitHooks()
{
    if (MH_Initialize() != MH_OK)
    {
        return false;
    }

    return true;
}