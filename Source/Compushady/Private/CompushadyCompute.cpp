// Copyright 2023 - Roberto De Ioris.


#include "CompushadyCompute.h"
#include "Compushady.h"
#include "Serialization/ArrayWriter.h"

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
		ErrorMessages = "Unable to add Unreal metadata to the shader";
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
	FShaderCode ShaderCode;

	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		FShaderResourceTable ShaderResourceTable;
		FArrayWriter Writer;
		Writer << ShaderResourceTable;

		Blob.Append(Writer);
		ShaderCode.GetWriteAccess().Append(ByteCode);
	}
	else if (RHIInterfaceType == ERHIInterfaceType::Vulkan)
	{
		Blob.Append(ByteCode);
	}
	else
	{
		return false;
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
		OnSignaled.ExecuteIfBound(false, "The Compute Shader is already running");
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