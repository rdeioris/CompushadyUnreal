// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadySharedFence.h"
#include "CompushadySharedBackend.h"
#include "RHI.h"
#include "RenderingThread.h"
#include "RHICommandList.h"

bool UCompushadySharedFence::Initialize(const FString& InSharedName)
{
    SharedName = InSharedName;

    const ERHIInterfaceType RHIType = RHIGetInterfaceType();
    CompushadyShared::FExportedHandle Exported;

#if COMPUSHADY_SHARED_WITH_D3D12
    if (RHIType == ERHIInterfaceType::D3D12)
    {
        NativeFence = CompushadyShared::D3D12::CreateSharedFence(InSharedName, Exported);
        bIsD3D12 = true;
    }
#endif
#if COMPUSHADY_SHARED_WITH_VULKAN
    if (RHIType == ERHIInterfaceType::Vulkan)
    {
        NativeFence = CompushadyShared::Vulkan::CreateSharedSemaphore(InSharedName, Exported);
        bIsD3D12 = false;
    }
#endif

    if (!NativeFence || !Exported.IsValid())
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("Failed to create a shared GPU fence '%s' for the current RHI."), *InSharedName);
        NativeFence = nullptr;
        return false;
    }

    SharedHandle = Exported.Handle;
    return true;
}

void UCompushadySharedFence::SignalOnGPU(const int64 Value)
{
    if (!NativeFence)
    {
        return;
    }

#if COMPUSHADY_SHARED_WITH_D3D12
    if (bIsD3D12)
    {
        CompushadyShared::D3D12::EnqueueSignal(NativeFence, static_cast<uint64>(Value));
        return;
    }
#endif
#if COMPUSHADY_SHARED_WITH_VULKAN
    if (!bIsD3D12)
    {
        CompushadyShared::Vulkan::EnqueueSignal(NativeFence, static_cast<uint64>(Value));
        return;
    }
#endif
}

void UCompushadySharedFence::WaitOnGPU(const int64 Value)
{
    if (!NativeFence)
    {
        return;
    }

#if COMPUSHADY_SHARED_WITH_D3D12
    if (bIsD3D12)
    {
        CompushadyShared::D3D12::EnqueueWait(NativeFence, static_cast<uint64>(Value));
        return;
    }
#endif
#if COMPUSHADY_SHARED_WITH_VULKAN
    if (!bIsD3D12)
    {
        CompushadyShared::Vulkan::EnqueueWait(NativeFence, static_cast<uint64>(Value));
        return;
    }
#endif
}

int64 UCompushadySharedFence::GetCompletedValue() const
{
    if (!NativeFence)
    {
        return 0;
    }

#if COMPUSHADY_SHARED_WITH_D3D12
    if (bIsD3D12)
    {
        return static_cast<int64>(CompushadyShared::D3D12::GetCompletedValue(NativeFence));
    }
#endif
#if COMPUSHADY_SHARED_WITH_VULKAN
    if (!bIsD3D12)
    {
        return static_cast<int64>(CompushadyShared::Vulkan::GetCompletedValue(NativeFence));
    }
#endif
    return 0;
}

void UCompushadySharedFence::BeginDestroy()
{
    // Release the native fence/semaphore on the render thread, not here on the game thread.
    // SignalOnGPU/WaitOnGPU enqueue render/RHI-thread work that captures the raw NativeFence pointer,
    // and the GPU queue keeps referencing it until those submissions retire. Releasing it inline
    // during GC could free the object out from under in-flight work (use-after-free). Deferring the
    // release behind a render command guarantees it runs after any signal/wait this fence enqueued.
    // The exported OS handle is closed in the same command purely to keep the teardown ordered; it
    // has no GPU dependency of its own.
    void* const FenceToRelease = NativeFence;
    void* const HandleToClose = SharedHandle;
    const bool bReleaseAsD3D12 = bIsD3D12;
    NativeFence = nullptr;
    SharedHandle = nullptr;

    if (FenceToRelease || HandleToClose)
    {
        ENQUEUE_RENDER_COMMAND(CompushadyReleaseSharedFence)(
            [FenceToRelease, HandleToClose, bReleaseAsD3D12](FRHICommandListImmediate&)
            {
#if COMPUSHADY_SHARED_WITH_D3D12
                if (bReleaseAsD3D12 && FenceToRelease)
                {
                    CompushadyShared::D3D12::ReleaseFence(FenceToRelease);
                }
#endif
#if COMPUSHADY_SHARED_WITH_VULKAN
                if (!bReleaseAsD3D12 && FenceToRelease)
                {
                    CompushadyShared::Vulkan::ReleaseSemaphore(FenceToRelease);
                }
#endif
                if (HandleToClose)
                {
                    CompushadyShared::ClosePlatformHandle(HandleToClose);
                }
            });
    }

    Super::BeginDestroy();
}
