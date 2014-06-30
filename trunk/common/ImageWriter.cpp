#include "ImageWriter.h"

#include "Common.h"
#include "Platform.h"

#include <ddraw.h>

#include <algorithm>
#include <cassert>

#pragma pack(push, 1)
struct TGAHeader
{
    uint8_t idLength;
    uint8_t colorMapType;
    uint8_t imageType;
    uint16_t colorMapOrigin;
    uint16_t colorMapLength;
    uint8_t colorMapEntrySize;
    uint16_t xOrigin;
    uint16_t yOrigin;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t imageDescriptor;
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

bool WriteTGA(const std::wstring& name, size_t width, size_t height, TextureFormat format, void* pData, int pitch)
{
    if (format != TextureFormat_ARGB
        && format != TextureFormat_XRGB)
    {
        return false;
    }

    unique_handle_ptr filePtr(CreateFile(name.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL),
                               &FileDeleter);
    if (filePtr.get() == INVALID_HANDLE_VALUE)
    {
        log(L"Failed to create file (%d, %s)", GetLastError(), name.c_str());
        return false;
    }

    TGAHeader header;
    header.idLength = 0;
    header.colorMapType = 0;
    header.imageType = 2;   // uncompressed RGB
    header.colorMapOrigin = 0;
    header.colorMapLength = 0;
    header.colorMapEntrySize = 0;
    header.xOrigin = 0;
    header.yOrigin = 0;
    header.width = static_cast<uint16_t>(width);
    header.height = static_cast<uint16_t>(height);
    header.bpp = 32;
    header.imageDescriptor = 0;

    DWORD bytesWritten = 0;
    if (!WriteFile(filePtr.get(), &header, sizeof(header), &bytesWritten, NULL))
    {
        log(L"Write operation failed (%d, %s)", GetLastError(), name.c_str());
        return false;
    }

    uint8_t* row = static_cast<uint8_t*>(pData);
    for (size_t i = 0; i < height; ++i)
    {
        bytesWritten = 0;
        if (!WriteFile(filePtr.get(), row, width * header.bpp / 8, &bytesWritten, NULL))
        {
            log(L"Write operation failed (%d, %d, %dx%d, %s)", GetLastError(), i, width, height, name.c_str());
            return false;
        }
        row += pitch;
    }

    return true;
}

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

bool WriteDDS(const std::wstring& name, size_t width, size_t height, TextureFormat format, void* pData, int pitch)
{
    if (format < TextureFormat_DXT1
        || format > TextureFormat_DXT5)
    {
        return false;
    }

    unique_handle_ptr filePtr(CreateFile(name.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL),
                                 &FileDeleter);
    if (filePtr.get() == INVALID_HANDLE_VALUE)
    {
        log(L"Failed to create file (%d, %s)", GetLastError(), name.c_str());
        return false;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(filePtr.get(), &DDS_MAGIC, sizeof(DDS_MAGIC), &bytesWritten, NULL))
    {
        log(L"Write operation failed (%d, %s)", GetLastError(), name.c_str());
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
        log(L"Write operation failed (%d, %s)", GetLastError(), name.c_str());
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
            log(L"Write operation failed (%d, %d, %dx%d(%d), %d, %s)", GetLastError(), i, width, height, normalizedHeight, format, name.c_str());
            return false;
        }
        row += pitch;
    }

    return true;
}