// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyShared.h"
#include "UObject/Object.h"
#include "CompushadySharedFence.generated.h"

/**
 * A GPU fence/timeline whose native object is shared across processes:
 *  - D3D12  : a shared ID3D12Fence (monotonic uint64 timeline), exported as a named NT handle.
 *  - Vulkan : an exportable timeline VkSemaphore, exported as an OPAQUE_WIN32 handle / OPAQUE_FD.
 *
 * The producer (this engine) signals the timeline on the GPU after a dispatch; the consumer waits
 * on the same value on its own device, and vice-versa. Values are monotonic — pick an incrementing
 * frame counter shared by both sides.
 *
 * Fence values are uint64 on the native side; they are surfaced as int64 for Blueprint.
 */
UCLASS(BlueprintType)
class COMPUSHADYSHARED_API UCompushadySharedFence : public UObject
{
    GENERATED_BODY()

public:
    // Create the native shared fence/semaphore for the current RHI and export its OS handle.
    bool Initialize(const FString& InSharedName);

    // The fence's shared-handle name — on D3D12 the kernel-object name the peer opens by (empty on
    // Vulkan, whose handles are transported by value). A fence has no separate RHI debug name.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady|Shared")
    FString GetSharedHandleName() const { return SharedName; }

    /** Raw OS handle value (NT HANDLE on Windows, fd on Linux). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady|Shared")
    int64 GetSharedHandle() const { return reinterpret_cast<int64>(SharedHandle); }

    // True if the native fence/semaphore was created successfully and this object is usable.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady|Shared")
    bool IsValidFence() const { return NativeFence != nullptr; }

    /**
     * Enqueue a GPU-side signal of the timeline to Value, ordered after all rendering commands
     * enqueued so far. The peer process can then wait on Value. Non-blocking.
     */
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    void SignalOnGPU(const int64 Value);

    /**
     * Enqueue a GPU-side wait until the timeline reaches Value (typically produced by the peer),
     * ordered before subsequent rendering commands. Non-blocking on the CPU.
     */
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    void WaitOnGPU(const int64 Value);

    /** CPU poll of the highest value the GPU has passed so far. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady|Shared")
    int64 GetCompletedValue() const;

    virtual void BeginDestroy() override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Compushady|Shared")
    FString SharedName;

    void* NativeFence = nullptr;   // ID3D12Fence* or VkSemaphore
    void* SharedHandle = nullptr;
    bool bIsD3D12 = false;         // which backend created NativeFence (fixed at Initialize)
};
