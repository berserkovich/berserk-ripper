#pragma once

#include "common/Common.h"

#include <map>

struct ImageInfo
{
    size_t width;
    size_t height;
    TextureFormat format;
    std::string filename;
};

class ImageCache
{
public:
    ImageCache(const std::string& path);
    ~ImageCache();

    std::string Add(size_t width, size_t height, TextureFormat format, void* pData, int pitch);

private:
    std::string ComputeHash_(const uint8_t* data, size_t length);
    bool WriteDDS_(const std::string& name, size_t width, size_t height, TextureFormat format, void* pData, int pitch);

private:
    std::map<std::string, ImageInfo> m_cache;
    std::string m_path;
};