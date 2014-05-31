#pragma once

#include <cstddef>

#include <functional>
#include <memory>

static const wchar_t FILE_MAPPING_OBJECT_NAME[] = L"BerserkRipperSharedObject";

struct D3D9DeviceOffsets
{
    ptrdiff_t Present;
};

typedef std::unique_ptr<void, std::function<void(void*)>> unique_sharedmem_ptr;
unique_sharedmem_ptr CreateSharedMemory(const wchar_t* name, size_t size);
unique_sharedmem_ptr OpenSHaredMemory(const wchar_t* name, size_t size);

void log(const wchar_t* format, ...);