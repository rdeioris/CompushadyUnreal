// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadySharedBackend.h"

#if COMPUSHADY_SHARED_WITH_D3D12

#include "ID3D12DynamicRHI.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "Runtime/Launch/Resources/Version.h"

#include "Windows/AllowWindowsPlatformTypes.h"

namespace
{
    ID3D12DynamicRHI* GetD3D12RHIChecked()
    {
        if (!IsRHID3D12())
        {
            return nullptr;
        }
        return GetID3D12DynamicRHI();
    }

    // Common named-handle creation for any ID3D12DeviceChild (resource or fence).
    void* CreateNamedSharedHandle(ID3D12Device* Device, ID3D12DeviceChild* Object, const FString& Name)
    {
        if (!Device || !Object)
        {
            return nullptr;
        }

        HANDLE Handle = nullptr;
        const TCHAR* HandleName = Name.IsEmpty() ? nullptr : *Name;
        const HRESULT Result = Device->CreateSharedHandle(Object, nullptr, GENERIC_ALL, HandleName, &Handle);
        if (FAILED(Result))
        {
            UE_LOG(LogCompushadyShared, Error, TEXT("[D3D12] CreateSharedHandle for '%s' failed (hr=0x%08x)"), *Name, (uint32)Result);
            return nullptr;
        }
        return Handle;
    }
}

namespace CompushadyShared
{
    namespace D3D12
    {
        FExportedHandle ExportResource(FRHITexture* Texture, const FString& Name)
        {
            FExportedHandle Out;

            ID3D12DynamicRHI* DynRHI = GetD3D12RHIChecked();
            if (!DynRHI || !Texture)
            {
                return Out;
            }

            if (DynRHI->RHIIsResourcePlaced(Texture))
            {
                UE_LOG(LogCompushadyShared, Error, TEXT("[D3D12] Texture '%s' is a placed (sub-allocated) resource and cannot be shared. Create it with the Shared flag."), *Name);
                return Out;
            }

            ID3D12Device* Device = DynRHI->RHIGetDevice(0);
            ID3D12Resource* Resource = DynRHI->RHIGetResource(Texture);
            Out.Handle = CreateNamedSharedHandle(Device, Resource, Name);
            Out.bValid = Out.Handle != nullptr;
            return Out;
        }

        FExportedHandle ExportResource(FRHIBuffer* Buffer, const FString& Name)
        {
            FExportedHandle Out;

            ID3D12DynamicRHI* DynRHI = GetD3D12RHIChecked();
            if (!DynRHI || !Buffer)
            {
                return Out;
            }

            if (DynRHI->RHIIsResourcePlaced(Buffer))
            {
                UE_LOG(LogCompushadyShared, Error, TEXT("[D3D12] Buffer '%s' is a placed (sub-allocated) resource and cannot be shared. Create it with the Shared flag."), *Name);
                return Out;
            }

            ID3D12Device* Device = DynRHI->RHIGetDevice(0);
            ID3D12Resource* Resource = DynRHI->RHIGetResource(Buffer);
            Out.Handle = CreateNamedSharedHandle(Device, Resource, Name);
            Out.bValid = Out.Handle != nullptr;
            return Out;
        }

        void* CreateSharedFence(const FString& Name, FExportedHandle& OutHandle)
        {
            OutHandle = FExportedHandle();

            ID3D12DynamicRHI* DynRHI = GetD3D12RHIChecked();
            if (!DynRHI)
            {
                return nullptr;
            }

            ID3D12Device* Device = DynRHI->RHIGetDevice(0);
            if (!Device)
            {
                return nullptr;
            }

            ID3D12Fence* Fence = nullptr;
            const HRESULT Result = Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&Fence));
            if (FAILED(Result) || !Fence)
            {
                UE_LOG(LogCompushadyShared, Error, TEXT("[D3D12] CreateFence for '%s' failed (hr=0x%08x)"), *Name, (uint32)Result);
                return nullptr;
            }

            OutHandle.Handle = CreateNamedSharedHandle(Device, Fence, Name);
            if (!OutHandle.Handle)
            {
                Fence->Release();
                return nullptr;
            }
            OutHandle.bValid = true;

            return Fence;
        }

        void ReleaseFence(void* Fence)
        {
            if (Fence)
            {
                reinterpret_cast<ID3D12Fence*>(Fence)->Release();
            }
        }

        void EnqueueSignal(void* Fence, uint64 Value)
        {
            if (!Fence || !IsRHID3D12())
            {
                return;
            }

            ID3D12Fence* NativeFence = reinterpret_cast<ID3D12Fence*>(Fence);
            ENQUEUE_RENDER_COMMAND(CompushadySharedD3D12Signal)(
                [NativeFence, Value](FRHICommandListImmediate& RHICmdList)
                {
                    // RHISignalManualFence -> RHICmdList.GetComputeContext(), which asserts a single
                    // active pipeline with a bound context. SwitchPipeline only *records* the activation
                    // (it binds the context at bottom-of-pipe), so the signal must run inside an
                    // EnqueueLambda that executes after that activation. GetComputeContext is a
                    // FRHICommandListBase method, so operating on the executing base list is sound.
                    FRHICommandListScopedPipeline ScopedPipeline(RHICmdList, ERHIPipeline::Graphics);
                    RHICmdList.EnqueueLambda([NativeFence, Value](FRHICommandListBase& ExecutingCmdList)
                        {
                            GetID3D12DynamicRHI()->RHISignalManualFence(static_cast<FRHICommandList&>(ExecutingCmdList), NativeFence, Value);
                        });
                });
        }

        void EnqueueWait(void* Fence, uint64 Value)
        {
            if (!Fence || !IsRHID3D12())
            {
                return;
            }

            ID3D12Fence* NativeFence = reinterpret_cast<ID3D12Fence*>(Fence);
            ENQUEUE_RENDER_COMMAND(CompushadySharedD3D12Wait)(
                [NativeFence, Value](FRHICommandListImmediate& RHICmdList)
                {
                    FRHICommandListScopedPipeline ScopedPipeline(RHICmdList, ERHIPipeline::Graphics);
                    RHICmdList.EnqueueLambda([NativeFence, Value](FRHICommandListBase& ExecutingCmdList)
                        {
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
                            // 5.8 added a fourth argument: a flag the RHI sets once the
                            // wait has been satisfied (it is stored in the payload's
                            // ManualFencesToWait). Nothing here observes it, but it must
                            // be a valid shared pointer rather than null.
                            GetID3D12DynamicRHI()->RHIWaitManualFence(static_cast<FRHICommandList&>(ExecutingCmdList), NativeFence, Value, MakeShared<std::atomic<bool>>(false));
#else
                            GetID3D12DynamicRHI()->RHIWaitManualFence(static_cast<FRHICommandList&>(ExecutingCmdList), NativeFence, Value);
#endif
                        });
                });
        }

        uint64 GetCompletedValue(void* Fence)
        {
            if (!Fence)
            {
                return 0;
            }
            return reinterpret_cast<ID3D12Fence*>(Fence)->GetCompletedValue();
        }
    }
}

#include "Windows/HideWindowsPlatformTypes.h"

#endif // COMPUSHADY_SHARED_WITH_D3D12
