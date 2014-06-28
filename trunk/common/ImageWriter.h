#pragma once

#include <string>

enum TextureFormat
{
    TextureFormat_Unknown,
    TextureFormat_ARGB,
    TextureFormat_XRGB
};

bool WriteTGA(const std::wstring& name, size_t width, size_t height, TextureFormat format, void* pData, int pitch);
