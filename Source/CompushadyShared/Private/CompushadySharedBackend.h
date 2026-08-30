// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyShared.h"
#include "RHIResources.h"

/**
 * Internal, RHI-specific backend surface.
 *
 * Each backend lives in its own translation unit so that windows.h (D3D12) and vulkan.h
 * (Vulkan) never share a compile unit. All functions are game/render-thread callable as noted
 * and never throw; failures are logged and reported through the return value / OutHandle validity.
 */
namespace CompushadyShared
{
    // Close a native OS handle exported by any backend. Windows: ::CloseHandle on the NT HANDLE.
    // Linux/Android: close() on the file descriptor packed in the pointer bits. No-op elsewhere.
    // Registered on UCompushadyResource so each resource can close its own exported handle.
    void ClosePlatformHandle(void* Handle);

    /** Result of exporting a native handle. */
    struct FExportedHandle
    {
        // Windows: an NT HANDLE (as void*). Linux/Vulkan: a file descriptor stored in the low pointer bits.
        void* Handle = nullptr;
        bool bValid = false;

        bool IsValid() const { return bValid; }
    };

#if COMPUSHADY_SHARED_WITH_D3D12
    namespace D3D12
    {
        // Export a shared NT handle for a texture/buffer previously created with TexCreate_Shared / BUF_Shared.
        // Must run on the render thread (touches the RHI resource). Name is used verbatim as the kernel object name.
        FExportedHandle ExportResource(FRHITexture* Texture, const FString& Name);
        FExportedHandle ExportResource(FRHIBuffer* Buffer, const FString& Name);

        // Create a shared ID3D12Fence. Returns the fence as an opaque pointer and fills OutHandle
        // with its named NT shared handle. Game-thread callable.
        void* CreateSharedFence(const FString& Name, FExportedHandle& OutHandle);
        void ReleaseFence(void* Fence);

        // GPU-timeline signal/wait, ordered relative to already-enqueued rendering commands.
        // Enqueue-only (no stall). Call from the game thread.
        void EnqueueSignal(void* Fence, uint64 Value);
        void EnqueueWait(void* Fence, uint64 Value);

        // CPU-side poll of the last value the GPU has passed. Any thread.
        uint64 GetCompletedValue(void* Fence);
    }
#endif

#if COMPUSHADY_SHARED_WITH_VULKAN
    namespace Vulkan
    {
        // Export the VkDeviceMemory backing the resource as an OPAQUE_WIN32 handle / OPAQUE_FD.
        // Requires the resource to have been allocated exportable (TexCreate_External / BUF_Shared).
        // Must run on the render thread. Name is informational only (Vulkan handles are not named).
        FExportedHandle ExportMemory(FRHITexture* Texture, const FString& Name);
        FExportedHandle ExportMemory(FRHIBuffer* Buffer, const FString& Name);

        // Create an exportable timeline semaphore. Returns it as an opaque pointer and fills OutHandle
        // with the exported OS handle/fd. Game-thread callable.
        void* CreateSharedSemaphore(const FString& Name, FExportedHandle& OutHandle);
        void ReleaseSemaphore(void* Semaphore);

        // GPU-timeline signal/wait via an empty queue submission on the RHI graphics queue.
        // Ordered after previously submitted work. Experimental.
        void EnqueueSignal(void* Semaphore, uint64 Value);
        void EnqueueWait(void* Semaphore, uint64 Value);

        // CPU-side poll of the current timeline value. Any thread.
        uint64 GetCompletedValue(void* Semaphore);
    }
#endif
}
