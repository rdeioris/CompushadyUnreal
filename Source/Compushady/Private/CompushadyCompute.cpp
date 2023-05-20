// Copyright 2023 - Roberto De Ioris.


#include "CompushadyCompute.h"
#include "Compushady.h"
#include "Serialization/ArrayWriter.h"
#include "vulkan.h"
#include "VulkanCommon.h"
#include "VulkanShaderResources.h"

bool UCompushadyCompute::InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	bRunning = false;

	RHIInterfaceType = RHIGetInterfaceType();

	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, "cs_6_0", ByteCode, ShaderResourceBindings, ErrorMessages))
	{
		return false;
	}

	uint32 NumCBVs = 0;
	// configure CBV resource bindings
	for (int32 Index = 0; Index < ShaderResourceBindings.CBVs.Num(); Index++)
	{
		Compushady::FCompushadyShaderResourceBinding& ShaderResourceBinding = ShaderResourceBindings.CBVs[Index];
		if (ShaderResourceBinding.SlotIndex + 1 > NumCBVs)
		{
			NumCBVs = ShaderResourceBinding.SlotIndex + 1;
		}

		FCompushadyResourceBinding ResourceBinding;
		ResourceBinding.SlotIndex = ShaderResourceBinding.SlotIndex;
		ResourceBinding.Name = ShaderResourceBinding.Name;
		CBVResourceBindings.Add(ResourceBinding);
		CBVResourceBindingsMap.Add(ResourceBinding.Name, ResourceBinding);
		CBVResourceBindingsSlotMap.Add(ResourceBinding.SlotIndex, ResourceBinding);
	}

	uint32 NumSRVs = 0;
	// configure SRV resource bindings
	for (int32 Index = 0; Index < ShaderResourceBindings.SRVs.Num(); Index++)
	{
		Compushady::FCompushadyShaderResourceBinding& ShaderResourceBinding = ShaderResourceBindings.SRVs[Index];
		if (ShaderResourceBinding.SlotIndex + 1 > NumSRVs)
		{
			NumSRVs = ShaderResourceBinding.SlotIndex + 1;
		}

		FCompushadyResourceBinding ResourceBinding;
		ResourceBinding.SlotIndex = ShaderResourceBinding.SlotIndex;
		ResourceBinding.Name = ShaderResourceBinding.Name;
		SRVResourceBindings.Add(ResourceBinding);
		SRVResourceBindingsMap.Add(ResourceBinding.Name, ResourceBinding);
		SRVResourceBindingsSlotMap.Add(ResourceBinding.SlotIndex, ResourceBinding);
	}

	uint32 NumUAVs = 0;
	// configure UAV resource bindings
	for (int32 Index = 0; Index < ShaderResourceBindings.UAVs.Num(); Index++)
	{
		Compushady::FCompushadyShaderResourceBinding& ShaderResourceBinding = ShaderResourceBindings.UAVs[Index];
		if (ShaderResourceBinding.SlotIndex + 1 > NumUAVs)
		{
			NumUAVs = ShaderResourceBinding.SlotIndex + 1;
		}

		FCompushadyResourceBinding ResourceBinding;
		ResourceBinding.SlotIndex = ShaderResourceBinding.SlotIndex;
		ResourceBinding.Name = ShaderResourceBinding.Name;
		UAVResourceBindings.Add(ResourceBinding);
		UAVResourceBindingsMap.Add(ResourceBinding.Name, ResourceBinding);
		UAVResourceBindingsSlotMap.Add(ResourceBinding.SlotIndex, ResourceBinding);
	}

	TArray<uint8> UnrealByteCode;
	if (!ToUnrealShader(ByteCode, UnrealByteCode, NumCBVs, NumSRVs, NumUAVs))
	{
		return false;
	}

	FSHA1 Sha1;
	Sha1.Update(UnrealByteCode.GetData(), UnrealByteCode.Num());
	FSHAHash Hash = Sha1.Finalize();

	ComputeShaderRef = RHICreateComputeShader(UnrealByteCode, Hash);
	if (!ComputeShaderRef.IsValid())
	{
		return false;
	}
	ComputeShaderRef->SetHash(Hash);

	ComputePipelineStateRef = RHICreateComputePipelineState(ComputeShaderRef);
	if (!ComputePipelineStateRef.IsValid())
	{
		return false;
	}

	FenceRef = RHICreateGPUFence(TEXT("CompushadyFence"));
	if (!FenceRef.IsValid())
	{
		return false;
	}

	return true;
}

bool UCompushadyCompute::ToUnrealShader(const TArray<uint8>& ByteCode, TArray<uint8>& Blob, const uint32 NumCBVs, const uint32 NumSRVs, const uint32 NumUAVs)
{
	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		FShaderResourceTable ShaderResourceTable;
		FArrayWriter Writer;
		Writer << ShaderResourceTable;

		Blob.Append(Writer);
	}
	else if (RHIInterfaceType == ERHIInterfaceType::Vulkan)
	{
		FVulkanShaderHeader VulkanShaderHeader;
		VulkanShaderHeader.SpirvCRC = FCrc::MemCrc32(ByteCode.GetData(), ByteCode.Num());

		// the entry point is fixed to "main_00000000_00000000"
		// update it to pass the size/crc check
		ANSICHAR EntryPoint[24];
		FCStringAnsi::Snprintf(EntryPoint, 24, "main_%0.8x_%0.8x", ByteCode.Num(), VulkanShaderHeader.SpirvCRC);


		// SPIR-V is generally managed as an array of 32bit words
		TArrayView<uint32> SpirV = TArrayView<uint32>((uint32*)ByteCode.GetData(), ByteCode.Num() / sizeof(uint32));

		// skip the first 4 words
		int32 Offset = 5;

		struct FCompushadySpirVDecoration
		{
			uint32 Binding;
			uint32 BindingIndexOffset;
			uint32 DescriptorSetOffset;
			FString ReflectionType;
		};

		// this is a map between SPIR-V ids bindings and reflection types
		TMap<uint32, FCompushadySpirVDecoration> Bindings;

		while (Offset < SpirV.Num())
		{
			uint32 Word = SpirV[Offset];
			uint16 Opcode = Word & 0xFFFF;
			uint16 Size = Word >> 16;
			if (Size == 0)
			{
				break;
			}

			// get the bindings/descriptor sets
			if (Opcode == 71 && (Offset + Size < SpirV.Num())) // OpDecorate(71) + id + Binding
			{
				if (Size > 3)
				{
					if (SpirV[Offset + 2] == 33) // Binding
					{
						FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 1]);
						Decoration.Binding = SpirV[Offset + 3];
						Decoration.BindingIndexOffset = Offset + 3;
					}
					else if (SpirV[Offset + 2] == 34) // DescriptorSet
					{
						FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 1]);
						Decoration.DescriptorSetOffset = Offset + 3;
					}
				}
			}
			// get the reflection friendly type
			else if (Opcode == 5632 && (Offset + Size < SpirV.Num())) // OpDecorateString(5632) + id + Decoration(UserTypeGOOGLE/5636) + String
			{
				if (Size > 3 && SpirV[Offset + 2] == 5636)
				{
					const char* DecorationString = reinterpret_cast<char*>(&SpirV[Offset + 3]);
					FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 1]);
					Decoration.ReflectionType = UTF8_TO_TCHAR(DecorationString);
				}
			}
			// patch the EntryPoint Name
			else if (Opcode == 15 && (Offset + Size < SpirV.Num())) // OpEntryPoint(15) + ExecutionModel + id + Name
			{
				if (Size > 9 && SpirV[Offset + 1] == 5) // ExecutionModel == GLCompute
				{
					uint32* EntryPointPtr = reinterpret_cast<uint32*>(EntryPoint);
					for (int32 Index = 0; Index < 6; Index++)
					{
						SpirV[Offset + 3 + Index] = EntryPointPtr[Index];
					}
				}
			}
			Offset += Size;
		}

		int32 UniformBufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::UniformBuffer);
		int32 ImageType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::Image);
		int32 StorageImageType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::StorageImage);

		for (const TPair<uint32, FCompushadySpirVDecoration>& Pair : Bindings)
		{

			FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
			GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
			GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;


			FVulkanShaderHeader::FSpirvInfo SpirvInfo;
			SpirvInfo.BindingIndexOffset = Pair.Value.BindingIndexOffset;
			SpirvInfo.DescriptorSetOffset = Pair.Value.DescriptorSetOffset;

			if (Pair.Value.Binding < 1024) // CBV
			{
				GlobalInfo.TypeIndex = UniformBufferType;
			}
			else if (Pair.Value.Binding < 2048) // SRV
			{
				GlobalInfo.TypeIndex = ImageType;
			}
			else if (Pair.Value.Binding < 3072) // UAV
			{
				GlobalInfo.TypeIndex = StorageImageType;
			}

			VulkanShaderHeader.Globals.Add(GlobalInfo);
			VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);


			UE_LOG(LogTemp, Error, TEXT("ID: %u Binding: %u BindingOffset: %u DescriptotSetOffset: %u ReflectionType: %s TypeIndex: %d"),
				Pair.Key,
				Pair.Value.Binding,
				Pair.Value.BindingIndexOffset,
				Pair.Value.DescriptorSetOffset,
				*Pair.Value.ReflectionType,
				GlobalInfo.TypeIndex);

		}

		FArrayWriter Writer;

		Writer << VulkanShaderHeader;

		int32 SpirvSize = ByteCode.Num();

		// a bit annoying, but we need to manage the const here
		Writer << SpirvSize;
		Writer.Serialize(const_cast<void*>(reinterpret_cast<const void*>(ByteCode.GetData())), ByteCode.Num());

		int32 MinusOne = -1;

		Writer << MinusOne;


		Blob.Append(Writer);
	}
	else
	{
		return false;
	}

	FShaderCode ShaderCode;
	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		ShaderCode.GetWriteAccess().Append(ByteCode);
	}

	FShaderCodePackedResourceCounts PackedResourceCounts = {};
	PackedResourceCounts.UsageFlags = EShaderResourceUsageFlags::GlobalUniformBuffer;
	PackedResourceCounts.NumCBs = NumCBVs;
	PackedResourceCounts.NumSRVs = NumSRVs;
	PackedResourceCounts.NumUAVs = NumUAVs;
	ShaderCode.AddOptionalData<FShaderCodePackedResourceCounts>(PackedResourceCounts);

	FShaderCodeResourceMasks ResourceMasks = {};
	ResourceMasks.UAVMask = 0xffffffff;
	ShaderCode.AddOptionalData<FShaderCodeResourceMasks>(ResourceMasks);

	Blob.Append(ShaderCode.GetReadAccess());

	return true;
}

void UCompushadyCompute::Dispatch(const FCompushadyResourceArray& ResourceArray, const int32 X, const int32 Y, const int32 Z, const FCompushadySignaled& OnSignaled)
{
	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "");
		return;
	}

	const TArray<UCompushadyCBV*>& CBVs = ResourceArray.CBVs;
	const TArray<UCompushadySRV*>& SRVs = ResourceArray.SRVs;
	const TArray<UCompushadyUAV*>& UAVs = ResourceArray.UAVs;

	if (CBVs.Num() != CBVResourceBindings.Num())
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected %d CBVs got %d"), CBVResourceBindings.Num(), CBVs.Num()));
		return;
	}

	if (SRVs.Num() != SRVResourceBindings.Num())
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected %d SRVs got %d"), SRVResourceBindings.Num(), SRVs.Num()));
		return;
	}

	if (UAVs.Num() != UAVResourceBindings.Num())
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected %d UAVs got %d"), UAVResourceBindings.Num(), UAVs.Num()));
		return;
	}

	bRunning = true;

	FenceRef->Clear();

	ENQUEUE_RENDER_COMMAND(DoCompushady)(
		[this, CBVs, SRVs, UAVs](FRHICommandListImmediate& RHICmdList)
		{


			SetComputePipelineState(RHICmdList, ComputeShaderRef);

	for (int32 Index = 0; Index < CBVs.Num(); Index++)
	{
		if (CBVs[Index]->BufferDataIsDirty())
		{
			CBVs[Index]->SyncBufferData(RHICmdList);
		}
		RHICmdList.SetShaderUniformBuffer(ComputeShaderRef, CBVResourceBindings[Index].SlotIndex, CBVs[Index]->GetRHI());
	}

	for (int32 Index = 0; Index < SRVs.Num(); Index++)
	{
		RHICmdList.Transition(SRVs[Index]->GetRHITransitionInfo());
		RHICmdList.SetShaderResourceViewParameter(ComputeShaderRef, SRVResourceBindings[Index].SlotIndex, SRVs[Index]->GetRHI());
	}

	for (int32 Index = 0; Index < UAVs.Num(); Index++)
	{
		RHICmdList.Transition(UAVs[Index]->GetRHITransitionInfo());
		RHICmdList.SetUAVParameter(ComputeShaderRef, UAVResourceBindings[Index].SlotIndex, UAVs[Index]->GetRHI());
	}

	RHICmdList.DispatchComputeShader(128, 128, 1);

	RHICmdList.WriteGPUFence(FenceRef);
		});

	CheckFence(OnSignaled);
}

void UCompushadyCompute::CheckFence(FCompushadySignaled OnSignal)
{
	if (!FenceRef->Poll())
	{
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, OnSignal]() { CheckFence(OnSignal); });
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [this, OnSignal]()
			{
				OnSignal.ExecuteIfBound(true, "");
		bRunning = false;
			});
	}
}

bool UCompushadyCompute::IsRunning() const
{
	return bRunning;
}