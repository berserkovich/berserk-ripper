#pragma once

#include "common/Platform.h"

#include <d3d9types.h>
#include <dxgiformat.h>

#include <cstdint>
#include <string>

void GetSurfaceSize(size_t width, size_t height, D3DFORMAT format, size_t* outNumBytes, size_t* outRowBytes, size_t* outNumRows);
bool SaveDDS(const std::string& name, size_t width, size_t height, D3DFORMAT format, void* pData, int pitch);
