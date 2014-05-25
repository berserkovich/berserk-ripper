#include "Common.h"

#include "Platform.h"

unique_sharedmem_ptr CreateSharedMemory(const wchar_t* name, size_t size)
{
    HANDLE hMapFile = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(D3D9DeviceOffsets), name);
    if (hMapFile == NULL)
    {
        return nullptr;
    }

    void* pSharedMemory = ::MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (pSharedMemory == NULL)
    {
        ::CloseHandle(hMapFile);
        return 0;
    }

    auto sharedMemDeleter = [=](void* ptr){ ::UnmapViewOfFile(ptr); ::CloseHandle(hMapFile); };
    return unique_sharedmem_ptr(pSharedMemory, sharedMemDeleter);
}

unique_sharedmem_ptr OpenSHaredMemory(const wchar_t* name, size_t size)
{
    HANDLE hMapFile = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (hMapFile == NULL)
    {
        return nullptr;
    }

    void* pSharedMemory = ::MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (pSharedMemory == NULL)
    {
        ::CloseHandle(hMapFile);
        return 0;
    }

    auto sharedMemDeleter = [=](void* ptr){ ::UnmapViewOfFile(ptr); ::CloseHandle(hMapFile); };
    return unique_sharedmem_ptr(pSharedMemory, sharedMemDeleter);
}
