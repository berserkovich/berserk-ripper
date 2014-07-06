#include "Common.h"

#include "Platform.h"

#include <Strsafe.h>

#include <vector>

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

unique_sharedmem_ptr OpenSharedMemory(const wchar_t* name, size_t size)
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

std::wstring Utf8ToWideChar(const std::string& utf8String)
{
    if (utf8String.empty())
    {
        return std::wstring();
    }

    std::vector<wchar_t> buffer(utf8String.size() + 1);
    MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, &buffer[0], buffer.size());
    return std::wstring(&buffer[0]);
}

std::string WideCharToUtf8(const std::wstring& wideString)
{
    if (wideString.empty())
    {
        return std::string();
    }

    std::vector<char> buffer(wideString.size() * 4 + 1);
    WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, &buffer[0], buffer.size(), NULL, NULL);
    return std::string(&buffer[0]);
}

void log(int line, const char* file, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf_s(buffer, 1024, format, args);
    std::wstring message = Utf8ToWideChar(buffer);
    OutputDebugString(message.c_str());
}
