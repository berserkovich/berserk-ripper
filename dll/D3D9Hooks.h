#pragma once

#include "common/Platform.h"

bool InitD3D9Hooks(HMODULE hModule);
void RevertD3D9Hooks();
