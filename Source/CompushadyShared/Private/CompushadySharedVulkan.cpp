// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadySharedBackend.h"

#if COMPUSHADY_SHARED_WITH_VULKAN

#include "IVulkanDynamicRHI.h"

// -----------------------------------------------------------------------------------------------
// Self-contained Vulkan ABI declarations.
//
// We only rely on the *opaque* core handle types that UE's Vulkan headers always expose
// (VkDevice, VkQueue, VkSemaphore, VkDeviceMemory, VkResult, VkStructureType, VKAPI_PTR).
// Every struct we pass to the driver is redeclared here with a Compushady-prefixed name and the
// exact Vulkan ABI layout, so this file compiles whether or not the platform KHR headers
// (vulkan_win32.h) are available in the build. All entry points are fetched at runtime through
// the RHI's device proc-address getter, so nothing needs to be link-resolved.
// -----------------------------------------------------------------------------------------------

namespace
{
    // sType values (from the Vulkan spec / registry).
    constexpr int32 CS_STYPE_SUBMIT_INFO = 4;
    constexpr int32 CS_STYPE_SEMAPHORE_CREATE_INFO = 9;
    constexpr int32 CS_STYPE_SEMAPHORE_TYPE_CREATE_INFO = 1000207002;
    constexpr int32 CS_STYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO = 1000207003;
    constexpr int32 CS_STYPE_EXPORT_SEMAPHORE_CREATE_INFO = 1000077000;
    constexpr int32 CS_STYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR = 1000078000;
    constexpr int32 CS_STYPE_SEMAPHORE_GET_FD_INFO_KHR = 1000079000;
    constexpr int32 CS_STYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR = 1000073003;
    constexpr int32 CS_STYPE_MEMORY_GET_FD_INFO_KHR = 1000074002;

    constexpr uint32 CS_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT = 0x00000001;
    constexpr uint32 CS_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT = 0x00000002;
    constexpr uint32 CS_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT = 0x00000001;
    constexpr uint32 CS_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT = 0x00000002;

    constexpr int32 CS_SEMAPHORE_TYPE_TIMELINE = 1;
    constexpr int32 CS_VK_SUCCESS = 0;

    // --- struct layouts (must match the Vulkan ABI exactly) ---

    struct FCS_SemaphoreTypeCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        int32 semaphoreType;   // VkSemaphoreType
        uint32 _pad;           // explicit padding before the 64-bit field
        uint64 initialValue;
    };

    struct FCS_ExportSemaphoreCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        uint32 handleTypes;    // VkExternalSemaphoreHandleTypeFlags
    };

    struct FCS_SemaphoreCreateInfo
    {
        VkStructureType sType;
        const void* pNext;
        uint32 flags;
    };

    struct FCS_TimelineSemaphoreSubmitInfo
    {
        VkStructureType sType;
        const void* pNext;
        uint32 waitSemaphoreValueCount;
        const uint64* pWaitSemaphoreValues;
        uint32 signalSemaphoreValueCount;
        const uint64* pSignalSemaphoreValues;
    };

    struct FCS_SubmitInfo
    {
        VkStructureType sType;
        const void* pNext;
        uint32 waitSemaphoreCount;
        const VkSemaphore* pWaitSemaphores;
        const uint32* pWaitDstStageMask;   // VkPipelineStageFlags*
        uint32 commandBufferCount;
        const void* pCommandBuffers;       // const VkCommandBuffer*
        uint32 signalSemaphoreCount;
        const VkSemaphore* pSignalSemaphores;
    };

    struct FCS_MemoryGetWin32HandleInfoKHR
    {
        VkStructureType sType;
        const void* pNext;
        VkDeviceMemory memory;
        uint32 handleType;   // VkExternalMemoryHandleTypeFlagBits
    };

    struct FCS_MemoryGetFdInfoKHR
    {
        VkStructureType sType;
        const void* pNext;
        VkDeviceMemory memory;
        uint32 handleType;
    };

    struct FCS_SemaphoreGetWin32HandleInfoKHR
    {
        VkStructureType sType;
        const void* pNext;
        VkSemaphore semaphore;
        uint32 handleType;   // VkExternalSemaphoreHandleTypeFlagBits
    };

    struct FCS_SemaphoreGetFdInfoKHR
    {
        VkStructureType sType;
        const void* pNext;
        VkSemaphore semaphore;
        uint32 handleType;
    };

    // --- entry-point typedefs ---
    typedef VkResult(VKAPI_PTR* PFN_CS_vkCreateSemaphore)(VkDevice, const FCS_SemaphoreCreateInfo*, const void*, VkSemaphore*);
    typedef void    (VKAPI_PTR* PFN_CS_vkDestroySemaphore)(VkDevice, VkSemaphore, const void*);
    typedef VkResult(VKAPI_PTR* PFN_CS_vkGetSemaphoreCounterValue)(VkDevice, VkSemaphore, uint64*);
    typedef VkResult(VKAPI_PTR* PFN_CS_vkQueueSubmit)(VkQueue, uint32, const FCS_SubmitInfo*, void* /*VkFence*/);
    typedef VkResult(VKAPI_PTR* PFN_CS_vkGetMemoryWin32HandleKHR)(VkDevice, const FCS_MemoryGetWin32HandleInfoKHR*, void** /*HANDLE*/);
    typedef VkResult(VKAPI_PTR* PFN_CS_vkGetMemoryFdKHR)(VkDevice, const FCS_MemoryGetFdInfoKHR*, int32*);
    typedef VkResult(VKAPI_PTR* PFN_CS_vkGetSemaphoreWin32HandleKHR)(VkDevice, const FCS_SemaphoreGetWin32HandleInfoKHR*, void** /*HANDLE*/);
    typedef VkResult(VKAPI_PTR* PFN_CS_vkGetSemaphoreFdKHR)(VkDevice, const FCS_SemaphoreGetFdInfoKHR*, int32*);

    template<typename T>
    T LoadDeviceProc(IVulkanDynamicRHI* DynRHI, const char* Name)
    {
        return reinterpret_cast<T>(DynRHI->RHIGetVkDeviceProcAddr(Name));
    }

    IVulkanDynamicRHI* GetVulkanRHIChecked()
    {
        if (!GDynamicRHI || GDynamicRHI->GetInterfaceType() != ERHIInterfaceType::Vulkan)
        {
            return nullptr;
        }
        return GetIVulkanDynamicRHI();
    }

    // Export a VkDeviceMemory as an OS handle. Returns true on success and fills OutHandle/bValid.
    bool ExportDeviceMemory(IVulkanDynamicRHI* DynRHI, VkDeviceMemory Memory, CompushadyShared::FExportedHandle& OutHandle)
    {
        VkDevice Device = DynRHI->RHIGetVkDevice();

#if PLATFORM_WINDOWS
        PFN_CS_vkGetMemoryWin32HandleKHR Fn = LoadDeviceProc<PFN_CS_vkGetMemoryWin32HandleKHR>(DynRHI, "vkGetMemoryWin32HandleKHR");
        if (!Fn)
        {
            UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] vkGetMemoryWin32HandleKHR unavailable (VK_KHR_external_memory_win32 not enabled)."));
            return false;
        }
        FCS_MemoryGetWin32HandleInfoKHR Info{};
        Info.sType = (VkStructureType)CS_STYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        Info.memory = Memory;
        Info.handleType = CS_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        void* Handle = nullptr;
        const VkResult Result = Fn(Device, &Info, &Handle);
        if (Result != CS_VK_SUCCESS || !Handle)
        {
            UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] vkGetMemoryWin32HandleKHR failed (VkResult=%d). Was the resource allocated exportable?"), (int32)Result);
            return false;
        }
        OutHandle.Handle = Handle;
        OutHandle.bValid = true;
        return true;
#else
        PFN_CS_vkGetMemoryFdKHR Fn = LoadDeviceProc<PFN_CS_vkGetMemoryFdKHR>(DynRHI, "vkGetMemoryFdKHR");
        if (!Fn)
        {
            UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] vkGetMemoryFdKHR unavailable (VK_KHR_external_memory_fd not enabled)."));
            return false;
        }
        FCS_MemoryGetFdInfoKHR Info{};
        Info.sType = (VkStructureType)CS_STYPE_MEMORY_GET_FD_INFO_KHR;
        Info.memory = Memory;
        Info.handleType = CS_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        int32 Fd = -1;
        const VkResult Result = Fn(Device, &Info, &Fd);
        if (Result != CS_VK_SUCCESS || Fd < 0)
        {
            UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] vkGetMemoryFdKHR failed (VkResult=%d). Was the resource allocated exportable?"), (int32)Result);
            return false;
        }
        OutHandle.Handle = reinterpret_cast<void*>(static_cast<intptr_t>(Fd));
        OutHandle.bValid = true;
        return true;
#endif
    }
}

namespace CompushadyShared
{
    namespace Vulkan
    {
        FExportedHandle ExportMemory(FRHITexture* Texture, const FString& Name)
        {
            FExportedHandle Out;

            IVulkanDynamicRHI* DynRHI = GetVulkanRHIChecked();
            if (!DynRHI || !Texture)
            {
                return Out;
            }

            FVulkanRHIAllocationInfo Alloc = DynRHI->RHIGetAllocationInfo(Texture);
            if (!Alloc.Handle)
            {
                UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] Texture '%s' has no VkDeviceMemory to export."), *Name);
                return Out;
            }

            ExportDeviceMemory(DynRHI, Alloc.Handle, Out);
            return Out;
        }

        FExportedHandle ExportMemory(FRHIBuffer* Buffer, const FString& Name)
        {
            FExportedHandle Out;

            IVulkanDynamicRHI* DynRHI = GetVulkanRHIChecked();
            if (!DynRHI || !Buffer)
            {
                return Out;
            }

            FVulkanRHIAllocationInfo Alloc = DynRHI->RHIGetAllocationInfo(Buffer);
            if (!Alloc.Handle)
            {
                UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] Buffer '%s' has no VkDeviceMemory to export."), *Name);
                return Out;
            }

            ExportDeviceMemory(DynRHI, Alloc.Handle, Out);
            return Out;
        }

        void* CreateSharedSemaphore(const FString& Name, FExportedHandle& OutHandle)
        {
            OutHandle = FExportedHandle();

            IVulkanDynamicRHI* DynRHI = GetVulkanRHIChecked();
            if (!DynRHI)
            {
                return nullptr;
            }

            VkDevice Device = DynRHI->RHIGetVkDevice();
            const void* AllocCallbacks = DynRHI->RHIGetVkAllocationCallbacks();

            PFN_CS_vkCreateSemaphore CreateSemaphore = LoadDeviceProc<PFN_CS_vkCreateSemaphore>(DynRHI, "vkCreateSemaphore");
            if (!CreateSemaphore)
            {
                UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] vkCreateSemaphore unavailable."));
                return nullptr;
            }

#if PLATFORM_WINDOWS
            const uint32 HandleTypeBit = CS_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
            const uint32 HandleTypeBit = CS_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

            FCS_ExportSemaphoreCreateInfo ExportInfo{};
            ExportInfo.sType = (VkStructureType)CS_STYPE_EXPORT_SEMAPHORE_CREATE_INFO;
            ExportInfo.handleTypes = HandleTypeBit;

            FCS_SemaphoreTypeCreateInfo TypeInfo{};
            TypeInfo.sType = (VkStructureType)CS_STYPE_SEMAPHORE_TYPE_CREATE_INFO;
            TypeInfo.pNext = &ExportInfo;
            TypeInfo.semaphoreType = CS_SEMAPHORE_TYPE_TIMELINE;
            TypeInfo.initialValue = 0;

            FCS_SemaphoreCreateInfo CreateInfo{};
            CreateInfo.sType = (VkStructureType)CS_STYPE_SEMAPHORE_CREATE_INFO;
            CreateInfo.pNext = &TypeInfo;

            VkSemaphore Semaphore = VK_NULL_HANDLE;
            const VkResult Result = CreateSemaphore(Device, &CreateInfo, AllocCallbacks, &Semaphore);
            if (Result != CS_VK_SUCCESS || Semaphore == VK_NULL_HANDLE)
            {
                UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] vkCreateSemaphore (exportable timeline) for '%s' failed (VkResult=%d)."), *Name, (int32)Result);
                return nullptr;
            }

            bool bExported = false;
#if PLATFORM_WINDOWS
            PFN_CS_vkGetSemaphoreWin32HandleKHR GetWin32 = LoadDeviceProc<PFN_CS_vkGetSemaphoreWin32HandleKHR>(DynRHI, "vkGetSemaphoreWin32HandleKHR");
            if (GetWin32)
            {
                FCS_SemaphoreGetWin32HandleInfoKHR Info{};
                Info.sType = (VkStructureType)CS_STYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
                Info.semaphore = Semaphore;
                Info.handleType = CS_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
                void* Handle = nullptr;
                if (GetWin32(Device, &Info, &Handle) == CS_VK_SUCCESS && Handle)
                {
                    OutHandle.Handle = Handle;
                    OutHandle.bValid = true;
                    bExported = true;
                }
            }
#else
            PFN_CS_vkGetSemaphoreFdKHR GetFd = LoadDeviceProc<PFN_CS_vkGetSemaphoreFdKHR>(DynRHI, "vkGetSemaphoreFdKHR");
            if (GetFd)
            {
                FCS_SemaphoreGetFdInfoKHR Info{};
                Info.sType = (VkStructureType)CS_STYPE_SEMAPHORE_GET_FD_INFO_KHR;
                Info.semaphore = Semaphore;
                Info.handleType = CS_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
                int32 Fd = -1;
                if (GetFd(Device, &Info, &Fd) == CS_VK_SUCCESS && Fd >= 0)
                {
                    OutHandle.Handle = reinterpret_cast<void*>(static_cast<intptr_t>(Fd));
                    OutHandle.bValid = true;
                    bExported = true;
                }
            }
#endif
            if (!bExported)
            {
                UE_LOG(LogCompushadyShared, Error, TEXT("[Vulkan] Failed to export shared handle for semaphore '%s'."), *Name);
                PFN_CS_vkDestroySemaphore DestroySemaphore = LoadDeviceProc<PFN_CS_vkDestroySemaphore>(DynRHI, "vkDestroySemaphore");
                if (DestroySemaphore)
                {
                    DestroySemaphore(Device, Semaphore, AllocCallbacks);
                }
                return nullptr;
            }

            return reinterpret_cast<void*>(Semaphore);
        }

        void ReleaseSemaphore(void* Semaphore)
        {
            IVulkanDynamicRHI* DynRHI = GetVulkanRHIChecked();
            if (!DynRHI || !Semaphore)
            {
                return;
            }
            PFN_CS_vkDestroySemaphore DestroySemaphore = LoadDeviceProc<PFN_CS_vkDestroySemaphore>(DynRHI, "vkDestroySemaphore");
            if (DestroySemaphore)
            {
                DestroySemaphore(DynRHI->RHIGetVkDevice(), reinterpret_cast<VkSemaphore>(Semaphore), DynRHI->RHIGetVkAllocationCallbacks());
            }
        }

        void EnqueueSignal(void* Semaphore, uint64 Value)
        {
            IVulkanDynamicRHI* DynRHI = GetVulkanRHIChecked();
            if (!DynRHI || !Semaphore)
            {
                return;
            }

            VkSemaphore NativeSemaphore = reinterpret_cast<VkSemaphore>(Semaphore);

            // Run an empty, signal-only submission on the RHI graphics queue. Because Vulkan queue
            // submissions execute in submission order, this signal is ordered after all work the RHI
            // has already submitted this frame.
            DynRHI->RHIRunOnQueue(EVulkanRHIRunOnQueueType::Graphics,
                [DynRHI, NativeSemaphore, Value](VkQueue Queue)
                {
                    PFN_CS_vkQueueSubmit QueueSubmit = LoadDeviceProc<PFN_CS_vkQueueSubmit>(DynRHI, "vkQueueSubmit");
                    if (!QueueSubmit)
                    {
                        return;
                    }

                    uint64 SignalValue = Value;
                    FCS_TimelineSemaphoreSubmitInfo TimelineInfo{};
                    TimelineInfo.sType = (VkStructureType)CS_STYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
                    TimelineInfo.signalSemaphoreValueCount = 1;
                    TimelineInfo.pSignalSemaphoreValues = &SignalValue;

                    FCS_SubmitInfo Submit{};
                    Submit.sType = (VkStructureType)CS_STYPE_SUBMIT_INFO;
                    Submit.pNext = &TimelineInfo;
                    Submit.signalSemaphoreCount = 1;
                    Submit.pSignalSemaphores = &NativeSemaphore;

                    QueueSubmit(Queue, 1, &Submit, nullptr);
                }, false);
        }

        void EnqueueWait(void* Semaphore, uint64 Value)
        {
            IVulkanDynamicRHI* DynRHI = GetVulkanRHIChecked();
            if (!DynRHI || !Semaphore)
            {
                return;
            }

            VkSemaphore NativeSemaphore = reinterpret_cast<VkSemaphore>(Semaphore);

            DynRHI->RHIRunOnQueue(EVulkanRHIRunOnQueueType::Graphics,
                [DynRHI, NativeSemaphore, Value](VkQueue Queue)
                {
                    PFN_CS_vkQueueSubmit QueueSubmit = LoadDeviceProc<PFN_CS_vkQueueSubmit>(DynRHI, "vkQueueSubmit");
                    if (!QueueSubmit)
                    {
                        return;
                    }

                    uint64 WaitValue = Value;
                    uint32 WaitStage = 0x00000001; // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT

                    FCS_TimelineSemaphoreSubmitInfo TimelineInfo{};
                    TimelineInfo.sType = (VkStructureType)CS_STYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
                    TimelineInfo.waitSemaphoreValueCount = 1;
                    TimelineInfo.pWaitSemaphoreValues = &WaitValue;

                    FCS_SubmitInfo Submit{};
                    Submit.sType = (VkStructureType)CS_STYPE_SUBMIT_INFO;
                    Submit.pNext = &TimelineInfo;
                    Submit.waitSemaphoreCount = 1;
                    Submit.pWaitSemaphores = &NativeSemaphore;
                    Submit.pWaitDstStageMask = &WaitStage;

                    QueueSubmit(Queue, 1, &Submit, nullptr);
                }, false);
        }

        uint64 GetCompletedValue(void* Semaphore)
        {
            IVulkanDynamicRHI* DynRHI = GetVulkanRHIChecked();
            if (!DynRHI || !Semaphore)
            {
                return 0;
            }
            PFN_CS_vkGetSemaphoreCounterValue GetValue = LoadDeviceProc<PFN_CS_vkGetSemaphoreCounterValue>(DynRHI, "vkGetSemaphoreCounterValue");
            if (!GetValue)
            {
                GetValue = LoadDeviceProc<PFN_CS_vkGetSemaphoreCounterValue>(DynRHI, "vkGetSemaphoreCounterValueKHR");
            }
            if (!GetValue)
            {
                return 0;
            }
            uint64 CounterValue = 0;
            GetValue(DynRHI->RHIGetVkDevice(), reinterpret_cast<VkSemaphore>(Semaphore), &CounterValue);
            return CounterValue;
        }
    }
}

#endif // COMPUSHADY_SHARED_WITH_VULKAN
