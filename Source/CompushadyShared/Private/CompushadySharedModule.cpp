// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadyShared.h"
#include "CompushadySharedBackend.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX || PLATFORM_ANDROID
#include <unistd.h>
#endif

DEFINE_LOG_CATEGORY(LogCompushadyShared);

namespace CompushadyShared
{
    void ClosePlatformHandle(void* Handle)
    {
        if (!Handle)
        {
            return;
        }
#if PLATFORM_WINDOWS
        ::CloseHandle(Handle);
#elif PLATFORM_LINUX || PLATFORM_ANDROID
        const int Fd = static_cast<int>(reinterpret_cast<intptr_t>(Handle));
        if (Fd >= 0)
        {
            close(Fd);
        }
#endif
    }
}

void FCompushadySharedModule::StartupModule()
{
}

void FCompushadySharedModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FCompushadySharedModule, CompushadyShared)
