#pragma once

#include "common/Common.h"
#include "common/Platform.h"

#include "MinHook.h"

#include <vector>

extern SharedData g_sharedData;

bool InitHooks();
void RevertHooks();

struct HookBase
{
    const ptrdiff_t& m_moduleOffset;

    HookBase(const ptrdiff_t& moduleOffset)
        : m_moduleOffset(moduleOffset)
    {

    }

    virtual bool CreateHook(HMODULE hModule) = 0;
    virtual void DisableHook() = 0;
    virtual void RemoveHook() = 0;
};

struct HookModule
{
    const char* name;
    std::vector<HookBase*> hooks;
};

template<typename F>
struct HookFunction
    : public HookBase
{
    F m_target;
    F m_real;
    F m_hook;

    HookFunction(HookModule& hookModule, const ptrdiff_t& moduleOffset, F hook)
        : HookBase(moduleOffset)
        , m_target(nullptr)
        , m_real(nullptr)
        , m_hook(hook)
    {
        hookModule.hooks.push_back(this);
    }

    bool CreateHook(HMODULE hModule)
    {
        m_target = reinterpret_cast<F>(reinterpret_cast<unsigned char*>(hModule) + m_moduleOffset);
        if (MH_CreateHook(m_target, m_hook, reinterpret_cast<void**>(&m_real)) != MH_OK
            || MH_QueueEnableHook(m_target) != MH_OK)
        {
            return false;
        }

        return true;
    }

    void DisableHook()
    {
        if (m_target)
        {
            MH_QueueDisableHook(m_target);
        }
    }

    void RemoveHook()
    {
        if (m_target)
        {
            MH_RemoveHook(m_target);
        }
    }
};

bool EnableModule(HookModule& hookModule);
void DisableModule(HookModule& hookModule);

#define DECLARE_HOOK_MODULE(moduleName)                                                  \
    HookModule g_##moduleName##Hooks = { #moduleName }

#define USE_HOOK_MODULE(moduleName)                                                      \
    extern HookModule g_##moduleName##Hooks

#define ENABLE_HOOK_MODULE(moduleName)                                                   \
    if (!EnableModule(g_##moduleName##Hooks)) return false

#define DISABLE_HOOK_MODULE(moduleName)                                                  \
    DisableModule(g_##moduleName##Hooks)

#define DECLARE_HOOK(moduleName, hookName, functionName, moduleOffset, ReturnType, ...)  \
    typedef ReturnType (__stdcall* functionName##Type)(__VA_ARGS__);                     \
    ReturnType __stdcall Hooked_##functionName(__VA_ARGS__);                             \
    HookFunction<functionName##Type> hookName = HookFunction<functionName##Type>(g_##moduleName##Hooks, moduleOffset, &Hooked_##functionName)
