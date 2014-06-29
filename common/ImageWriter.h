#pragma once

#include <string>

enum TextureFormat
{
    TextureFormat_Unknown,
    TextureFormat_ARGB,
    TextureFormat_XRGB,
    TextureFormat_DXT1,
    TextureFormat_DXT2,
    TextureFormat_DXT3,
    TextureFormat_DXT4,
    TextureFormat_DXT5
};

bool WriteTGA(const std::wstring& name, size_t width, size_t height, TextureFormat format, void* pData, int pitch);
bool WriteDDS(const std::wstring& name, size_t width, size_t height, TextureFormat format, void* pData, int pitch);
