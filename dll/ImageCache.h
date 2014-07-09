#pragma once

#include "common/Common.h"
#include "common/DDS.h"

#include <map>

class ImageCache
{
public:
    ImageCache(const std::string& path);
    ~ImageCache();

    std::string Add(size_t width, size_t height, D3DFORMAT format, void* pData, int pitch);

private:
    std::string ComputeHash_(const uint8_t* data, size_t length);

private:
    std::map<std::string, std::string> m_cache;
    std::string m_path;
};