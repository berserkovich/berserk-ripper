#pragma once

#include <cstddef>

#include <functional>
#include <memory>

static const wchar_t SHARED_MUTEX_NAME[] = L"BerserkRipperSharedMutex";
static const wchar_t FILE_MAPPING_OBJECT_NAME[] = L"BerserkRipperSharedObject";

struct D3D9DeviceOffsets
{
    ptrdiff_t DrawIndexedPrimitive;
    ptrdiff_t DrawIndexedPrimitiveUP;
    ptrdiff_t DrawPrimitive;
    ptrdiff_t DrawPrimitiveUP;
    ptrdiff_t DrawRectPatch;
    ptrdiff_t DrawTriPatch;
    ptrdiff_t Present;
};

struct SharedData
{
    D3D9DeviceOffsets d3d9DeviceOffsets;
};

typedef std::unique_ptr<void, std::function<void(void*)>> unique_sharedmem_ptr;
unique_sharedmem_ptr CreateSharedMemory(const wchar_t* name, size_t size);
unique_sharedmem_ptr OpenSharedMemory(const wchar_t* name, size_t size);

void log(const wchar_t* format, ...);