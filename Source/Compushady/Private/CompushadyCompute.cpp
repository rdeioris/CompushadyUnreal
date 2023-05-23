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
		ResourceBinding.BindingIndex = ShaderResourceBinding.BindingIndex;
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
		ResourceBinding.BindingIndex = ShaderResourceBinding.BindingIndex;
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
		ResourceBinding.BindingIndex = ShaderResourceBinding.BindingIndex;
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
	if (!ComputeShaderRef.IsValid() || !ComputeShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Compute Shader";
		return false;
	}
	ComputeShaderRef->SetHash(Hash);

	UE_LOG(LogTemp, Error, TEXT("CBV: %d SRV: %d UAV: %d Size: %d"), NumCBVs, NumSRVs, NumUAVs, UnrealByteCode.Num());

	ComputePipelineStateRef = RHICreateComputePipelineState(ComputeShaderRef);
	if (!ComputePipelineStateRef.IsValid() || !ComputePipelineStateRef->IsValid())
	{
		ErrorMessages = "Unable to create Compute Pipeline State";
		return false;
	}

	if (!InitFence(this))
	{
		ErrorMessages = "Unable to create Compute Fence";
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

	for (int32 Index = 0; Index < CBVResourceBindings.Num(); Index++)
	{
		if (!CBVs[Index])
		{
			const FCompushadyResourceBinding& ResourceBinding = CBVResourceBindings[Index];
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("CBV b%u (%s) at slot %u is NULL"), ResourceBinding.BindingIndex, *ResourceBinding.Name, Index));
			return;
		}
	}

	for (int32 Index = 0; Index < SRVResourceBindings.Num(); Index++)
	{
		if (!SRVs[Index])
		{
			const FCompushadyResourceBinding& ResourceBinding = SRVResourceBindings[Index];
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("CBV t%u (%s) at slot %u is NULL"), ResourceBinding.BindingIndex, *ResourceBinding.Name, Index));
			return;
		}
	}

	for (int32 Index = 0; Index < UAVResourceBindings.Num(); Index++)
	{
		if (!UAVs[Index])
		{
			const FCompushadyResourceBinding& ResourceBinding = UAVResourceBindings[Index];
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("CBV u%u (%s) at slot %u is NULL"), ResourceBinding.BindingIndex, *ResourceBinding.Name, Index));
			return;
		}
	}

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushady)(
		[this, CBVs, SRVs, UAVs, X, Y, Z](FRHICommandListImmediate& RHICmdList)
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

	RHICmdList.DispatchComputeShader(X, Y, Z);

	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

bool ICompushadySignalable::InitFence(UObject* InOwningObject)
{
	OwningObject = InOwningObject;

	FenceRef = RHICreateGPUFence(TEXT("CompushadyFence"));
	if (!FenceRef.IsValid() || !FenceRef->IsValid())
	{
		return false;
	}

	return true;
}

void ICompushadySignalable::WriteFence(FRHICommandListImmediate& RHICmdList)
{
	RHICmdList.WriteGPUFence(FenceRef);
}

void ICompushadySignalable::ClearFence()
{
	bRunning = true;
	FenceRef->Clear();
}

void ICompushadySignalable::CheckFence(FCompushadySignaled OnSignal)
{
	if (!OwningObject.IsValid(false, true))
	{
		return;
	}

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