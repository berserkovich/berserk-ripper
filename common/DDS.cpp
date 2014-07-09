#include "DDS.h"

#include "common/Common.h"

#include <algorithm>

#pragma pack(push, 1)

static const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

struct DDS_PIXELFORMAT
{
    uint32_t  size;
    uint32_t  flags;
    uint32_t  fourCC;
    uint32_t  RGBBitCount;
    uint32_t  RBitMask;
    uint32_t  GBitMask;
    uint32_t  BBitMask;
    uint32_t  ABitMask;
};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_LUMINANCEA  0x00020001  // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_PAL8        0x00000020  // DDPF_PALETTEINDEXED8

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                              DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                              DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ)

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

#define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME

typedef struct
{
    uint32_t          size;
    uint32_t          flags;
    uint32_t          height;
    uint32_t          width;
    uint32_t          pitchOrLinearSize;
    uint32_t          depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
    uint32_t          mipMapCount;
    uint32_t          reserved1[11];
    DDS_PIXELFORMAT   ddspf;
    uint32_t          caps;
    uint32_t          caps2;
    uint32_t          caps3;
    uint32_t          caps4;
    uint32_t          reserved2;
} DDS_HEADER;

typedef struct
{
    DXGI_FORMAT   dxgiFormat;
    uint32_t      resourceDimension;
    uint32_t      miscFlag; // see D3D11_RESOURCE_MISC_FLAG
    uint32_t      arraySize;
    uint32_t      reserved;
} DDS_HEADER_DXT10;

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

    bool IsFourCC(D3DFORMAT format)
    {
        switch (format)
        {
        case D3DFMT_UYVY:
        case D3DFMT_R8G8_B8G8:
        case D3DFMT_YUY2:
        case D3DFMT_G8R8_G8B8:
        case D3DFMT_DXT1:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_R16F:
        case D3DFMT_G16R16F:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_R32F:
        case D3DFMT_G32R32F:
        case D3DFMT_A32B32G32R32F:
        case D3DFMT_CxV8U8:
            return true;
        }

        return false;
    }

    size_t BitsPerPixel(D3DFORMAT format)
    {
        switch (format)
        {
        case D3DFMT_A32B32G32R32F:
            return 128;
        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_G32R32F:
            return 64;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A8B8G8R8:
        case D3DFMT_X8B8G8R8:
        case D3DFMT_G16R16:
        case D3DFMT_A2R10G10B10:
        case D3DFMT_X8L8V8U8:
        case D3DFMT_Q8W8V8U8:
        case D3DFMT_V16U16:
        case D3DFMT_A2W10V10U10:
        case D3DFMT_D32:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D24X4S4:
        case D3DFMT_D32F_LOCKABLE:
        case D3DFMT_D24FS8:
        case D3DFMT_D32_LOCKABLE:
        case D3DFMT_A2B10G10R10_XR_BIAS:
        case D3DFMT_G16R16F:
        case D3DFMT_R32F:
        case D3DFMT_R8G8_B8G8:
        case D3DFMT_G8R8_G8B8:
            return 32;
        case D3DFMT_R8G8B8:
            return 24;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_X4R4G4B4:
        case D3DFMT_A8P8:
        case D3DFMT_A8L8:
        case D3DFMT_V8U8:
        case D3DFMT_L6V5U5:
        case D3DFMT_D16_LOCKABLE:
        case D3DFMT_D15S1:
        case D3DFMT_D16:
        case D3DFMT_L16:
        case D3DFMT_R16F:
            return 16;
        case D3DFMT_R3G3B2:
        case D3DFMT_A8:
        case D3DFMT_P8:
        case D3DFMT_L8:
        case D3DFMT_A4L4:
        case D3DFMT_S8_LOCKABLE:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            return 8;
        case D3DFMT_DXT1:
            return 4;
        case D3DFMT_A1:
            return 1;

        //?? D3DFMT_MULTI2_ARGB8 = MAKEFOURCC('M', 'E', 'T', '1'),
        //?? D3DFMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'),
        //?? D3DFMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'),
        //?? D3DFMT_CxV8U8 = 117,
        }
        return 0;
    }

    uint32_t GetColorMask(D3DFORMAT format, uint32_t* outBitCount, size_t* outRbitMask, size_t* outGbitMask, size_t* outBbitMask, size_t* outABitMask)
    {
        *outRbitMask = 0;
        *outGbitMask = 0;
        *outBbitMask = 0;
        *outABitMask = 0;

        switch (format)
        {
        case D3DFMT_A8B8G8R8:
            *outBitCount = 32;
            *outRbitMask = 0xff;
            *outGbitMask = 0xff00;
            *outBbitMask = 0xff0000;
            *outABitMask = 0xff000000;
            return DDS_RGBA;
        case D3DFMT_X8B8G8R8:
            *outBitCount = 32;
            *outRbitMask = 0xff;
            *outGbitMask = 0xff00;
            *outBbitMask = 0xff0000;
            return DDS_RGBA;
        case D3DFMT_G16R16:
            *outBitCount = 32;
            *outRbitMask = 0xffff;
            *outGbitMask = 0xffff0000;
            return DDS_RGBA;
        case D3DFMT_A2B10G10R10:
            *outBitCount = 32;
            *outRbitMask = 0x3ff;
            *outGbitMask = 0xffc00;
            *outBbitMask = 0x3ff00000;
            *outABitMask = 0xc0000000;
            return DDS_RGBA;
        case D3DFMT_A2R10G10B10:
            *outBitCount = 32;
            *outRbitMask = 0x3ff00000;
            *outGbitMask = 0xffc00;
            *outBbitMask = 0x3ff;
            *outABitMask = 0xc0000000;
            return DDS_RGBA;
        case D3DFMT_A1R5G5B5:
            *outBitCount = 16;
            *outRbitMask = 0x7c00;
            *outGbitMask = 0x3e0;
            *outBbitMask = 0x1f;
            *outABitMask = 0x8000;
            return DDS_RGBA;
        case D3DFMT_X1R5G5B5:
            *outBitCount = 16;
            *outRbitMask = 0x7c00;
            *outGbitMask = 0x3e0;
            *outBbitMask = 0x1f;
            return DDS_RGBA;
        case D3DFMT_R5G6B5:
            *outBitCount = 16;
            *outRbitMask = 0xf800;
            *outGbitMask = 0x7e0;
            *outBbitMask = 0x1f;
            return DDS_RGB;
        case D3DFMT_A8:
            *outBitCount = 8;
            *outABitMask = 0xff;
            return DDS_ALPHA;
        case D3DFMT_A8R8G8B8:
            *outBitCount = 32;
            *outRbitMask = 0xff0000;
            *outGbitMask = 0xff00;
            *outBbitMask = 0xff;
            *outABitMask = 0xff000000;
            return DDS_RGBA;
        case D3DFMT_X8R8G8B8:
            *outBitCount = 32;
            *outRbitMask = 0xff0000;
            *outGbitMask = 0xff00;
            *outBbitMask = 0xff;
            return DDS_RGBA;
        case D3DFMT_R8G8B8:
            *outBitCount = 24;
            *outRbitMask = 0xff0000;
            *outGbitMask = 0xff00;
            *outBbitMask = 0xff;
            return DDS_RGB;
        case D3DFMT_A4R4G4B4:
            *outBitCount = 16;
            *outRbitMask = 0xf00;
            *outGbitMask = 0xf0;
            *outBbitMask = 0xf;
            *outABitMask = 0xf000;
            return DDS_RGBA;
        case D3DFMT_X4R4G4B4:
            *outBitCount = 16;
            *outRbitMask = 0xf00;
            *outGbitMask = 0xf0;
            *outBbitMask = 0xf;
            return DDS_RGBA;
        case D3DFMT_A8R3G3B2:
            *outBitCount = 16;
            *outRbitMask = 0xe0;
            *outGbitMask = 0x1c;
            *outBbitMask = 0x3;
            *outABitMask = 0xff00;
            return DDS_RGBA;
        case D3DFMT_A8L8:
            *outBitCount = 16;
            *outRbitMask = 0xff;
            *outABitMask = 0xff00;
            return DDS_LUMINANCE;
        case D3DFMT_L16:
            *outBitCount = 16;
            *outRbitMask = 0xffff;
            return DDS_LUMINANCE;
        case D3DFMT_L8:
            *outBitCount = 8;
            *outRbitMask = 0xff;
            return DDS_LUMINANCE;
        case D3DFMT_A4L4:
            *outBitCount = 8;
            *outRbitMask = 0xf;
            *outABitMask = 0xf0;
            return DDS_LUMINANCE;
        }

        return 0;
    }
}

typedef std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&FileDeleter)> unique_handle_ptr;

void GetSurfaceSize(size_t width, size_t height, D3DFORMAT format, size_t* outNumBytes, size_t* outRowBytes, size_t* outNumRows)
{
    size_t rowBytes = 0;
    size_t numRows = 0;
    switch (format)
    {
    case D3DFMT_DXT1:
        rowBytes = std::max<size_t>(1, (width + 3) / 4) * 8;
        numRows = std::max<size_t>(1, (height + 3) / 4);
        break;
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        rowBytes = std::max<size_t>(1, (width + 3) / 4) * 16;
        numRows = std::max<size_t>(1, (height + 3) / 4);
        break;
    case D3DFMT_R8G8_B8G8:
    case D3DFMT_G8R8_G8B8:
        rowBytes = ((width + 1) >> 1) * 4;
        numRows = height;
        break;
    default:
        rowBytes = (width * BitsPerPixel(format) + 7) / 8;
        numRows = height;
    }

    if (outNumBytes)
    {
        *outNumBytes = rowBytes * numRows;
    }
    if (outRowBytes)
    {
        *outRowBytes = rowBytes;
    }
    if (outNumRows)
    {
        *outNumRows = numRows;
    }
}

bool SaveDDS(const std::string& name, size_t width, size_t height, D3DFORMAT format, void* pData, int pitch)
{
    DDS_HEADER header = {};
    header.size = sizeof(header);
    header.flags = DDS_HEADER_FLAGS_TEXTURE;
    header.height = static_cast<DWORD>(height);
    header.width = static_cast<DWORD>(width);
    header.ddspf.size = sizeof(DDS_PIXELFORMAT);
    header.caps = DDS_SURFACE_FLAGS_TEXTURE;
    if (IsFourCC(format))
    {
        header.ddspf.flags = DDS_FOURCC;
        header.ddspf.fourCC = format;
    }
    else
    {
        header.ddspf.flags = GetColorMask(format, &header.ddspf.RGBBitCount, &header.ddspf.RBitMask, &header.ddspf.GBitMask, &header.ddspf.BBitMask, &header.ddspf.ABitMask);
        if (!header.ddspf.flags)
        {
            LOG("Unsupported format %d", format);
            return false;
        }
    }

    std::wstring wideName = Utf8ToWideChar(name);
    unique_handle_ptr filePtr(CreateFile(wideName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL), &FileDeleter);
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

    bytesWritten = 0;
    if (!WriteFile(filePtr.get(), &header, sizeof(header), &bytesWritten, NULL))
    {
        LOG("Write operation failed (%d, %s)", GetLastError(), name.c_str());
        return false;
    }

    size_t numBytes = 0;
    size_t rowBytes = 0;
    size_t numRows = 0;
    GetSurfaceSize(width, height, format, &numBytes, &rowBytes, &numRows);
    uint8_t* row = static_cast<uint8_t*>(pData);
    for (size_t i = 0; i < numRows; ++i)
    {
        bytesWritten = 0;
        if (!WriteFile(filePtr.get(), row, rowBytes, &bytesWritten, NULL))
        {
            LOG("Write operation failed (%d, %d, %dx%d(%d), %d, %s)", GetLastError(), i, width, height, numRows, format, name.c_str());
            return false;
        }
        row += pitch;
    }

    return true;
}
