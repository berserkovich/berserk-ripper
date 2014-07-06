#include "ImageCache.h"

#include "common/md5.h"
#include "common/Platform.h"

#include <ddraw.h>

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>

static const DWORD DDS_MAGIC = 0x20534444;

#pragma pack(push, 1)
struct DDS_PIXELFORMAT
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwABitMask;
};

struct DDS_HEADER
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwHeight;
    DWORD           dwWidth;
    DWORD           dwPitchOrLinearSize;
    DWORD           dwDepth;
    DWORD           dwMipMapCount;
    DWORD           dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    DWORD           dwCaps;
    DWORD           dwCaps2;
    DWORD           dwCaps3;
    DWORD           dwCaps4;
    DWORD           dwReserved2;
};
#pragma pack(pop)

namespace
{
    void FileDeleter(HANDLE hFile)
    {
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
        }
    }
}

typedef std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&FileDeleter)> unique_handle_ptr;

ImageCache::ImageCache(const std::string& path)
    : m_path(path)
{

}

ImageCache::~ImageCache()
{

}

std::string ImageCache::Add(size_t width, size_t height, TextureFormat format, void* pData, int pitch)
{
    size_t imageSize = pitch * height;
    if (format >= TextureFormat_DXT1 && format <= TextureFormat_DXT5)
    {
        imageSize /= 4;
    }

    std::string hash = ComputeHash_(static_cast<const uint8_t*>(pData), imageSize);
    auto it = m_cache.find(hash);
    if (it != m_cache.end())
    {
        return it->second.filename;
    }

    std::string filename = m_path + "\\" + hash + ".dds";
    WriteDDS_(filename, width, height, format, pData, pitch);
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

bool ImageCache::WriteDDS_(const std::string& name, size_t width, size_t height, TextureFormat format, void* pData, int pitch)
{
    std::wstring wideName = Utf8ToWideChar(name);
    unique_handle_ptr filePtr(CreateFile(wideName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL),
        &FileDeleter);
    if (filePtr.get() == INVALID_HANDLE_VALUE)
    {
        LOG("Failed to create file (%d, %s)", GetLastError(), name.c_str());
        return false;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(filePtr.get(), &DDS_MAGIC, sizeof(DDS_MAGIC), &bytesWritten, NULL))
    {
        LOG("Write operation failed (%d, %s)", GetLastError(), name.c_str());
        return false;
    }

    DDS_HEADER header;
    header.dwSize = sizeof(header);
    header.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    header.dwHeight = static_cast<DWORD>(height);
    header.dwWidth = static_cast<DWORD>(width);
    header.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
    header.ddspf.dwFlags = DDPF_FOURCC;
    header.ddspf.dwFourCC = MAKEFOURCC('D', 'X', 'T', '1' + format - TextureFormat_DXT1);
    header.dwCaps = DDSCAPS_TEXTURE;

    bytesWritten = 0;
    if (!WriteFile(filePtr.get(), &header, sizeof(header), &bytesWritten, NULL))
    {
        LOG("Write operation failed (%d, %s)", GetLastError(), name.c_str());
        return false;
    }

    size_t normalizedPitch = (std::max)(1u, (width + 3) / 4) * ((format == TextureFormat_DXT1) ? 8 : 16);
    size_t normalizedHeight = height / 4;
    uint8_t* row = static_cast<uint8_t*>(pData);
    for (size_t i = 0; i < normalizedHeight; ++i)
    {
        bytesWritten = 0;
        if (!WriteFile(filePtr.get(), row, normalizedPitch, &bytesWritten, NULL))
        {
            LOG("Write operation failed (%d, %d, %dx%d(%d), %d, %s)", GetLastError(), i, width, height, normalizedHeight, format, name.c_str());
            return false;
        }
        row += pitch;
    }

    return true;
}
