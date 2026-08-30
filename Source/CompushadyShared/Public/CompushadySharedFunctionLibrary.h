// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PixelFormat.h"
#include "CompushadyShared.h"
#include "CompushadySharedFence.h"
#include "CompushadySharedFunctionLibrary.generated.h"

class UCompushadyUAV;
class UCompushadySRV;

/**
 * Sharing helpers built on the *plain* Compushady view types.
 *
 * There is no dedicated shared-resource UObject: the creators below return ordinary
 * UCompushadyUAV / UCompushadySRV (so they drop straight into any Compushady pipeline). The exported
 * native OS handle is owned by the returned resource itself — it stays open for the resource's
 * lifetime (so a peer can keep opening it by name) and is closed automatically in the resource's
 * BeginDestroy when it is garbage-collected.
 *
 * Pair the resources with a UCompushadySharedFence for cross-process GPU synchronization.
 */
UCLASS()
class COMPUSHADYSHARED_API UCompushadySharedFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Every creator takes two names: Name is the RHI debug name given to the resource, and SharedName
    // is the cross-process kernel-object name a peer opens by (OpenSharedHandleByName on D3D12).

    // Create a UAV over a 2D texture allocated with the RHI-appropriate sharing flag (D3D12: Shared,
    // Vulkan: External) and export its native handle. Returns a plain UCompushadyUAV.
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    static UCompushadyUAV* CreateCompushadySharedUAVTexture2D(const FString& Name, const FString& SharedName, const int32 Width, const int32 Height, const EPixelFormat Format);

    // Create a UAV over a typed buffer (BUF_Shared) and export its native handle.
    // NOTE: buffer sharing is D3D12-only. On Vulkan this returns nullptr (buffer memory is not
    // exportable through the RHI there — share a texture instead). Even on D3D12 the buffer must be
    // committed to be shareable; small pool-suballocated (placed) buffers are rejected at export and
    // this returns nullptr. Prefer shared textures when a resource must work on both RHIs.
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    static UCompushadyUAV* CreateCompushadySharedUAVBuffer(const FString& Name, const FString& SharedName, const int64 Size, const EPixelFormat Format);

    // Create a UAV over a structured buffer (BUF_Shared) and export its native handle.
    // Same D3D12-only / committed-resource caveats as CreateCompushadySharedUAVBuffer above.
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    static UCompushadyUAV* CreateCompushadySharedUAVStructuredBuffer(const FString& Name, const FString& SharedName, const int64 Size, const int32 Stride);

    // Create an SRV over a shareable texture (typically written by the external process) and export
    // its native handle. One creator per texture dimension.
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    static UCompushadySRV* CreateCompushadySharedSRVTexture2D(const FString& Name, const FString& SharedName, const int32 Width, const int32 Height, const EPixelFormat Format);

    // Create an SRV over a shareable 3D (volume) texture and export its native handle.
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    static UCompushadySRV* CreateCompushadySharedSRVTexture3D(const FString& Name, const FString& SharedName, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format);

    // Create an SRV over a shareable cube texture (6 square faces of the given size) and export its native handle.
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    static UCompushadySRV* CreateCompushadySharedSRVTextureCube(const FString& Name, const FString& SharedName, const int32 Width, const EPixelFormat Format);

    // Create an SRV over a shareable 2D texture array (Slices layers) and export its native handle.
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    static UCompushadySRV* CreateCompushadySharedSRVTexture2DArray(const FString& Name, const FString& SharedName, const int32 Width, const int32 Height, const int32 Slices, const EPixelFormat Format);

    // Note: whether a resource is shared and its handle name are queried directly on the resource —
    // UCompushadyResource::IsShared() / GetSharedHandleName().

    // Create a shared GPU fence (D3D12 fence / Vulkan timeline semaphore) exported to a handle.
    UFUNCTION(BlueprintCallable, Category = "Compushady|Shared")
    static UCompushadySharedFence* CreateCompushadySharedFence(const FString& SharedName);
};
