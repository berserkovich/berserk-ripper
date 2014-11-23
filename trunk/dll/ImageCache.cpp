#include "ImageCache.h"

#include "common/md5.h"
#include "common/Platform.h"

#include <algorithm>

ImageCache::ImageCache(const std::string& path)
    : m_path(path)
{

}

ImageCache::~ImageCache()
{

}

std::string ImageCache::Add(size_t width, size_t height, D3DFORMAT format, void* pData, int pitch)
{
    size_t imageSize = 0;
    GetSurfaceSize(width, height, format, &imageSize, nullptr, nullptr);

    std::string hash = ComputeHash_(static_cast<const uint8_t*>(pData), imageSize);
    //auto it = m_cache.find(hash);
    //if (it != m_cache.end())
    //{
    //    return it->second;
    //}

    std::string filename = hash + ".dds";
    std::string fullpath = m_path + "\\" + filename;
    const bool flipY = true;
    if (SaveDDS(fullpath, width, height, format, pData, pitch, flipY))
    {
        m_cache.insert(std::make_pair(hash, filename));
    }
    return filename;
}

std::string ImageCache::ComputeHash_(const uint8_t* data, size_t length)
{
    uint8_t hash[16];
    MD5_CTX md5ctx;
    MD5Init(&md5ctx);
    MD5Update(&md5ctx, const_cast<unsigned char*>(data), length);
    MD5Final(hash, &md5ctx);

    static const std::string hex_char = "0123456789ABCDEF";
    std::string hashString;
    hashString.reserve(32);
    for (size_t i = 0; i < 16; ++i)
    {
        hashString.push_back(hex_char[(hash[i] >> 4) & 0x0F]);
        hashString.push_back(hex_char[hash[i] & 0x0F]);
    }
    return hashString;
}
