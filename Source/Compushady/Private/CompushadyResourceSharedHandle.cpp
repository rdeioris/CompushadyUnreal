// Copyright 2023-2026 - Roberto De Ioris.

// Isolated translation unit for closing a resource's exported cross-process handle.
//
// UCompushadyResource stores a native OS handle (an NT HANDLE on Windows, a file descriptor on
// Linux) that the CompushadyShared module exported for it. Closing that handle is the only place
// the core module needs platform headers, so it is kept here — on its own, away from the rest of
// CompushadyTypes.cpp — to avoid leaking <windows.h> macros into that large file.

#include "CompushadyTypes.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX || PLATFORM_ANDROID
#include <unistd.h>
#endif

void UCompushadyResource::CloseSharedHandle()
{
	if (SharedHandle)
	{
#if PLATFORM_WINDOWS
		::CloseHandle(SharedHandle);
#elif PLATFORM_LINUX || PLATFORM_ANDROID
		const int Fd = static_cast<int>(reinterpret_cast<intptr_t>(SharedHandle));
		if (Fd >= 0)
		{
			close(Fd);
		}
#endif
	}

	// SharedName is left intact — it is set from the factory, not tied to the handle's lifetime.
	SharedHandle = nullptr;
}
