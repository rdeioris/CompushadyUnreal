// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadySharedFunctionLibrary.h"
#include "CompushadyShared.h"
#include "CompushadySharedBackend.h"
#include "CompushadyUAV.h"
#include "CompushadySRV.h"
#include "Compushady.h"
#include "RHI.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RHIResources.h"

// ------------------------------------------------------------------------------------------------
// The exported native handle is owned by the resource itself (UCompushadyResource::SetSharedHandle):
// it stays open for the resource's lifetime and is closed in the resource's BeginDestroy. There is
// no side-table to keep in sync — this module only exports the handle and hands it to the resource.
// ------------------------------------------------------------------------------------------------

namespace
{
    // Export the native handle for a resource's underlying RHI texture/buffer (render-thread work).
    bool ExportResourceHandle(UCompushadyResource* Resource, const FString& Name, CompushadyShared::FExportedHandle& OutExported)
    {
        FTextureRHIRef TextureRHIRef = Resource->GetTextureRHI();
        FBufferRHIRef BufferRHIRef = Resource->GetBufferRHI();

        if (!TextureRHIRef.IsValid() && !BufferRHIRef.IsValid())
        {
            UE_LOG(LogCompushadyShared, Error, TEXT("Resource '%s' has no RHI texture or buffer to share."), *Name);
            return false;
        }

        ENQUEUE_RENDER_COMMAND(DoCompushadyExportSharedResource)(
            [&OutExported, TextureRHIRef, BufferRHIRef, Name](FRHICommandListImmediate& RHICmdList)
            {
                const ERHIInterfaceType RHIType = RHIGetInterfaceType();
#if COMPUSHADY_SHARED_WITH_D3D12
                if (RHIType == ERHIInterfaceType::D3D12)
                {
                    OutExported = TextureRHIRef.IsValid()
                        ? CompushadyShared::D3D12::ExportResource(TextureRHIRef.GetReference(), Name)
                        : CompushadyShared::D3D12::ExportResource(BufferRHIRef.GetReference(), Name);
                    return;
                }
#endif
#if COMPUSHADY_SHARED_WITH_VULKAN
                if (RHIType == ERHIInterfaceType::Vulkan)
                {
                    OutExported = TextureRHIRef.IsValid()
                        ? CompushadyShared::Vulkan::ExportMemory(TextureRHIRef.GetReference(), Name)
                        : CompushadyShared::Vulkan::ExportMemory(BufferRHIRef.GetReference(), Name);
                    return;
                }
#endif
            });

        FlushRenderingCommands();
        return OutExported.IsValid();
    }

    // Export the handle (under SharedName) and hand ownership to the resource. Returns false on failure.
    bool ShareResourceInternal(UCompushadyResource* Resource, const FString& SharedName)
    {
        if (!Resource)
        {
            return false;
        }

        CompushadyShared::FExportedHandle Exported;
        if (!ExportResourceHandle(Resource, SharedName, Exported))
        {
            UE_LOG(LogCompushadyShared, Warning, TEXT("Resource could not be exported for sharing as '%s' on the current RHI."), *SharedName);
            return false;
        }

        // The resource now owns the handle and will close it in BeginDestroy.
        Resource->SetSharedName(SharedName);
        Resource->SetSharedHandle(Exported.Handle);
        return true;
    }

    bool CurrentRHISupportsSharing()
    {
        const ERHIInterfaceType RHIType = RHIGetInterfaceType();
        return RHIType == ERHIInterfaceType::D3D12 || RHIType == ERHIInterfaceType::Vulkan;
    }

    // Buffer sharing is D3D12-only. On the Vulkan RHI, buffers are always pool-suballocated
    // (VulkanBuffer.cpp -> AllocateBufferPooled) and never allocated with VkExportMemoryAllocateInfo,
    // and there is no EBufferUsageFlags::External to request it (unlike textures, which honor
    // TexCreate_External). So a Vulkan "shared" buffer's VkDeviceMemory is not exportable — fail
    // fast instead of handing back a resource whose handle export silently fails. Shared *textures*
    // work on both RHIs. (Even on D3D12, only buffers large enough to bypass the pool are committed
    // and therefore shareable; smaller UAV buffers are placed resources and are rejected at export.)
    bool CurrentRHISupportsBufferSharing(const TCHAR* What, const FString& Name)
    {
        if (RHIGetInterfaceType() == ERHIInterfaceType::Vulkan)
        {
            UE_LOG(LogCompushadyShared, Error,
                TEXT("[Vulkan] Shared %s are not supported on the Vulkan RHI (buffer memory is not exportable there). '%s' was not created — share a texture instead, or run on D3D12."),
                What, *Name);
            return false;
        }
        return true;
    }

    // Extra texture flag needed for exportable allocation on the running RHI.
    ETextureCreateFlags SharingTextureFlag()
    {
        const ERHIInterfaceType RHIType = RHIGetInterfaceType();
        if (RHIType == ERHIInterfaceType::Vulkan)
        {
            return ETextureCreateFlags::External; // Vulkan-only: dedicated, exportable memory.
        }
        return ETextureCreateFlags::Shared;       // D3D12 (and the generic default).
    }

    // Create an RHI texture from a prepared descriptor, adding the RHI-appropriate sharing flag on
    // top of the caller's base flags (ShaderResource, +UAV for writable textures).
    FTextureRHIRef CreateSharedTexture(FRHITextureCreateDesc TextureCreateDesc, const ETextureCreateFlags BaseFlags)
    {
        TextureCreateDesc.SetFlags(BaseFlags | SharingTextureFlag());

        FTextureRHIRef TextureRHIRef = nullptr;
        ENQUEUE_RENDER_COMMAND(DoCompushadyCreateSharedTexture)(
            [&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
            {
                TextureRHIRef = RHICreateTexture(TextureCreateDesc);
            });
        FlushRenderingCommands();
        return TextureRHIRef;
    }

    // Wrap a freshly-created shared texture (any dimension) in an SRV and export it under SharedName.
    UCompushadySRV* MakeSharedSRV(FTextureRHIRef TextureRHIRef, const FString& SharedName)
    {
        if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
        {
            return nullptr;
        }

        UCompushadySRV* SRV = NewObject<UCompushadySRV>();
        if (!SRV->InitializeFromTexture(TextureRHIRef))
        {
            return nullptr;
        }

        if (!ShareResourceInternal(SRV, SharedName))
        {
            return nullptr;
        }
        return SRV;
    }
}

// ------------------------------------------------------------------------------------------------
// Blueprint API
// ------------------------------------------------------------------------------------------------

UCompushadyUAV* UCompushadySharedFunctionLibrary::CreateCompushadySharedUAVTexture2D(const FString& Name, const FString& SharedName, const int32 Width, const int32 Height, const EPixelFormat Format)
{
    if (Width <= 0 || Height <= 0 || !GPixelFormats[Format].Supported)
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("Invalid texture params for '%s' (%dx%d, format %s)."), *Name, Width, Height, GetPixelFormatString(Format));
        return nullptr;
    }
    if (!CurrentRHISupportsSharing())
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("The current RHI does not support cross-process sharing (only D3D12 and Vulkan)."));
        return nullptr;
    }

    FTextureRHIRef TextureRHIRef = CreateSharedTexture(
        FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format),
        ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV);
    if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
    {
        return nullptr;
    }

    UCompushadyUAV* UAV = NewObject<UCompushadyUAV>();
    if (!UAV->InitializeFromTexture(TextureRHIRef))
    {
        return nullptr;
    }

    if (!ShareResourceInternal(UAV, SharedName))
    {
        return nullptr;
    }
    return UAV;
}

UCompushadyUAV* UCompushadySharedFunctionLibrary::CreateCompushadySharedUAVBuffer(const FString& Name, const FString& SharedName, const int64 Size, const EPixelFormat Format)
{
    if (Size <= 0 || !GPixelFormats[Format].Supported)
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("Invalid buffer params for '%s' (size %lld, format %s)."), *Name, Size, GetPixelFormatString(Format));
        return nullptr;
    }
    if (!CurrentRHISupportsSharing())
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("The current RHI does not support cross-process sharing (only D3D12 and Vulkan)."));
        return nullptr;
    }
    if (!CurrentRHISupportsBufferSharing(TEXT("typed buffers"), Name))
    {
        return nullptr;
    }

    const int32 Stride = GPixelFormats[Format].BlockBytes;
    FBufferRHIRef BufferRHIRef;
    ENQUEUE_RENDER_COMMAND(DoCompushadyCreateSharedBuffer)(
        [&BufferRHIRef, Name, Size, Stride](FRHICommandListImmediate& RHICmdList)
        {
            BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Size,
                EBufferUsageFlags::ShaderResource | EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Shared,
                Stride, ERHIAccess::UAVMask);
        });
    FlushRenderingCommands();

    if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
    {
        return nullptr;
    }

    UCompushadyUAV* UAV = NewObject<UCompushadyUAV>();
    if (!UAV->InitializeFromBuffer(BufferRHIRef, Format))
    {
        return nullptr;
    }

    if (!ShareResourceInternal(UAV, SharedName))
    {
        return nullptr;
    }
    return UAV;
}

UCompushadyUAV* UCompushadySharedFunctionLibrary::CreateCompushadySharedUAVStructuredBuffer(const FString& Name, const FString& SharedName, const int64 Size, const int32 Stride)
{
    if (Size <= 0 || Stride <= 0)
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("Invalid structured buffer params for '%s' (size %lld, stride %d)."), *Name, Size, Stride);
        return nullptr;
    }
    if (!CurrentRHISupportsSharing())
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("The current RHI does not support cross-process sharing (only D3D12 and Vulkan)."));
        return nullptr;
    }
    if (!CurrentRHISupportsBufferSharing(TEXT("structured buffers"), Name))
    {
        return nullptr;
    }

    FBufferRHIRef BufferRHIRef;
    ENQUEUE_RENDER_COMMAND(DoCompushadyCreateSharedStructuredBuffer)(
        [&BufferRHIRef, Name, Size, Stride](FRHICommandListImmediate& RHICmdList)
        {
            BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Size,
                EBufferUsageFlags::ShaderResource | EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::StructuredBuffer | EBufferUsageFlags::Shared,
                Stride, ERHIAccess::UAVCompute);
        });
    FlushRenderingCommands();

    if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
    {
        return nullptr;
    }

    UCompushadyUAV* UAV = NewObject<UCompushadyUAV>();
    if (!UAV->InitializeFromStructuredBuffer(BufferRHIRef))
    {
        return nullptr;
    }

    if (!ShareResourceInternal(UAV, SharedName))
    {
        return nullptr;
    }
    return UAV;
}

UCompushadySRV* UCompushadySharedFunctionLibrary::CreateCompushadySharedSRVTexture2D(const FString& Name, const FString& SharedName, const int32 Width, const int32 Height, const EPixelFormat Format)
{
    if (Width <= 0 || Height <= 0 || !GPixelFormats[Format].Supported)
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("Invalid texture params for '%s' (%dx%d, format %s)."), *Name, Width, Height, GetPixelFormatString(Format));
        return nullptr;
    }
    if (!CurrentRHISupportsSharing())
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("The current RHI does not support cross-process sharing (only D3D12 and Vulkan)."));
        return nullptr;
    }

    return MakeSharedSRV(CreateSharedTexture(
        FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format),
        ETextureCreateFlags::ShaderResource), SharedName);
}

UCompushadySRV* UCompushadySharedFunctionLibrary::CreateCompushadySharedSRVTexture3D(const FString& Name, const FString& SharedName, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format)
{
    if (Width <= 0 || Height <= 0 || Depth <= 0 || !GPixelFormats[Format].Supported)
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("Invalid texture params for '%s' (%dx%dx%d, format %s)."), *Name, Width, Height, Depth, GetPixelFormatString(Format));
        return nullptr;
    }
    if (!CurrentRHISupportsSharing())
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("The current RHI does not support cross-process sharing (only D3D12 and Vulkan)."));
        return nullptr;
    }

    return MakeSharedSRV(CreateSharedTexture(
        FRHITextureCreateDesc::Create3D(*Name, Width, Height, Depth, Format),
        ETextureCreateFlags::ShaderResource), SharedName);
}

UCompushadySRV* UCompushadySharedFunctionLibrary::CreateCompushadySharedSRVTextureCube(const FString& Name, const FString& SharedName, const int32 Width, const EPixelFormat Format)
{
    if (Width <= 0 || !GPixelFormats[Format].Supported)
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("Invalid texture params for '%s' (%d, format %s)."), *Name, Width, GetPixelFormatString(Format));
        return nullptr;
    }
    if (!CurrentRHISupportsSharing())
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("The current RHI does not support cross-process sharing (only D3D12 and Vulkan)."));
        return nullptr;
    }

    return MakeSharedSRV(CreateSharedTexture(
        FRHITextureCreateDesc::CreateCube(*Name, Width, Format),
        ETextureCreateFlags::ShaderResource), SharedName);
}

UCompushadySRV* UCompushadySharedFunctionLibrary::CreateCompushadySharedSRVTexture2DArray(const FString& Name, const FString& SharedName, const int32 Width, const int32 Height, const int32 Slices, const EPixelFormat Format)
{
    if (Width <= 0 || Height <= 0 || Slices <= 0 || !GPixelFormats[Format].Supported)
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("Invalid texture params for '%s' (%dx%d x%d slices, format %s)."), *Name, Width, Height, Slices, GetPixelFormatString(Format));
        return nullptr;
    }
    if (!CurrentRHISupportsSharing())
    {
        UE_LOG(LogCompushadyShared, Error, TEXT("The current RHI does not support cross-process sharing (only D3D12 and Vulkan)."));
        return nullptr;
    }

    return MakeSharedSRV(CreateSharedTexture(
        FRHITextureCreateDesc::Create2DArray(*Name, Width, Height, Slices, Format),
        ETextureCreateFlags::ShaderResource), SharedName);
}

// Shared-state queries (IsShared / GetSharedHandleName) live on UCompushadyResource itself.
// Closing is automatic in the resource's BeginDestroy.

UCompushadySharedFence* UCompushadySharedFunctionLibrary::CreateCompushadySharedFence(const FString& SharedName)
{
    UCompushadySharedFence* SharedFence = NewObject<UCompushadySharedFence>();
    if (!SharedFence->Initialize(SharedName))
    {
        return nullptr;
    }
    return SharedFence;
}
