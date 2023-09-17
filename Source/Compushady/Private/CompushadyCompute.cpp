// Copyright 2023 - Roberto De Ioris.


#include "CompushadyCompute.h"
#include "Compushady.h"
#include "Serialization/ArrayWriter.h"

bool UCompushadyCompute::InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	RHIInterfaceType = RHIGetInterfaceType();

	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, "cs_6_0", ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	if (RHIInterfaceType == ERHIInterfaceType::Vulkan)
	{
		SPIRV = ByteCode;
	}
	else if (RHIInterfaceType == ERHIInterfaceType::Metal)
	{
		SPIRV = ByteCode;
	}
	else if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		DXIL = ByteCode;
	}

	return CreateComputePipeline(ByteCode, ShaderResourceBindings, ErrorMessages);
}

bool UCompushadyCompute::InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	RHIInterfaceType = RHIGetInterfaceType();

	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;
	if (!Compushady::CompileGLSL(ShaderCode, EntryPoint, "cs_6_0", ByteCode, ErrorMessages))
	{
		return false;
	}

	SPIRV = ByteCode;

	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		FString HLSL;
		if (!Compushady::SPIRVToHLSL(SPIRV, HLSL, ErrorMessages))
		{
			return false;
		}
		TStringBuilderBase<UTF8CHAR> SourceUTF8;
		SourceUTF8 = TCHAR_TO_UTF8(*HLSL);

		TArray<uint8> HLSLShaderCode;
		HLSLShaderCode.Append(reinterpret_cast<const uint8*>(*SourceUTF8), SourceUTF8.Len());
		return InitFromHLSL(HLSLShaderCode, EntryPoint, ErrorMessages);
	}
	else
	{
		if (!FixupSPIRV(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
		{
			return false;
		}
	}

	return CreateComputePipeline(ByteCode, ShaderResourceBindings, ErrorMessages);
}

bool UCompushadyCompute::InitFromSPIRV(const TArray<uint8>& ShaderCode, FString& ErrorMessages)
{
	RHIInterfaceType = RHIGetInterfaceType();

	if (RHIInterfaceType != ERHIInterfaceType::Vulkan)
	{
		ErrorMessages = "SPIRV shaders are currently supported only on Vulkan";
		return false;
	}

	TArray<uint8> ByteCode = ShaderCode;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;
	SPIRV = ByteCode;
	if (!Compushady::FixupSPIRV(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	return CreateComputePipeline(ByteCode, ShaderResourceBindings, ErrorMessages);
}

bool UCompushadyCompute::InitFromDXIL(const TArray<uint8>& ShaderCode, FString& ErrorMessages)
{
	RHIInterfaceType = RHIGetInterfaceType();

	if (RHIInterfaceType != ERHIInterfaceType::D3D12)
	{
		ErrorMessages = "DXIL shaders are currently supported only on Direct3D12";
		return false;
	}

	TArray<uint8> ByteCode = ShaderCode;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;
	DXIL = ByteCode;

	if (!Compushady::FixupDXIL(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	return CreateComputePipeline(ByteCode, ShaderResourceBindings, ErrorMessages);
}

bool UCompushadyCompute::CreateComputePipeline(TArray<uint8>& ByteCode, Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings, FString& ErrorMessages)
{
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

	// check for holes in the CBVs
	for (int32 Index = 0; Index < static_cast<int32>(NumCBVs); Index++)
	{
		bool bFound = false;
		for (const FCompushadyResourceBinding& Binding : CBVResourceBindings)
		{
			if (Binding.SlotIndex == Index)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			ErrorMessages = "Binding holes not allowed in CBVs";
			return false;
		}
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
	if (!Compushady::ToUnrealShader(ByteCode, UnrealByteCode, NumCBVs, NumSRVs, NumUAVs))
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

	ComputePipelineStateRef = RHICreateComputePipelineState(ComputeShaderRef);
	if (!ComputePipelineStateRef.IsValid() || !ComputePipelineStateRef->IsValid())
	{
		ErrorMessages = "Unable to create Compute Pipeline State";
		return false;
	}

	InitFence(this);

	return true;
}

bool UCompushadyCompute::SetupDispatch(const FCompushadyResourceArray& ResourceArray, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Compute Shader is already running");
		return false;
	}

	const TArray<UCompushadyCBV*>& CBVs = ResourceArray.CBVs;
	const TArray<UCompushadySRV*>& SRVs = ResourceArray.SRVs;
	const TArray<UCompushadyUAV*>& UAVs = ResourceArray.UAVs;

	if (CBVs.Num() != CBVResourceBindings.Num())
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected %d CBVs got %d"), CBVResourceBindings.Num(), CBVs.Num()));
		return false;
	}

	if (SRVs.Num() != SRVResourceBindings.Num())
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected %d SRVs got %d"), SRVResourceBindings.Num(), SRVs.Num()));
		return false;
	}

	if (UAVs.Num() != UAVResourceBindings.Num())
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected %d UAVs got %d"), UAVResourceBindings.Num(), UAVs.Num()));
		return false;
	}

	for (int32 Index = 0; Index < CBVResourceBindings.Num(); Index++)
	{
		if (!CBVs[Index])
		{
			const FCompushadyResourceBinding& ResourceBinding = CBVResourceBindings[Index];
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("CBV b%u (%s) at slot %u is NULL"), ResourceBinding.BindingIndex, *ResourceBinding.Name, Index));
			return false;
		}
	}

	for (int32 Index = 0; Index < SRVResourceBindings.Num(); Index++)
	{
		if (!SRVs[Index])
		{
			const FCompushadyResourceBinding& ResourceBinding = SRVResourceBindings[Index];
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("CBV t%u (%s) at slot %u is NULL"), ResourceBinding.BindingIndex, *ResourceBinding.Name, Index));
			return false;
		}
	}

	for (int32 Index = 0; Index < UAVResourceBindings.Num(); Index++)
	{
		if (!UAVs[Index])
		{
			const FCompushadyResourceBinding& ResourceBinding = UAVResourceBindings[Index];
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("CBV u%u (%s) at slot %u is NULL"), ResourceBinding.BindingIndex, *ResourceBinding.Name, Index));
			return false;
		}
	}

	return true;
}

void UCompushadyCompute::SetupPipeline(FRHICommandListImmediate& RHICmdList, const TArray<UCompushadyCBV*>& CBVs, const TArray<UCompushadySRV*>& SRVs, const TArray<UCompushadyUAV*>& UAVs)
{
	SetComputePipelineState(RHICmdList, ComputeShaderRef);
#if COMPUSHADY_UE_VERSION >= 53
	FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
#endif

	for (int32 Index = 0; Index < CBVs.Num(); Index++)
	{
		if (CBVs[Index]->BufferDataIsDirty())
		{
			CBVs[Index]->SyncBufferData(RHICmdList);
		}
#if COMPUSHADY_UE_VERSION >= 53
		BatchedParameters.SetShaderUniformBuffer(CBVResourceBindings[Index].SlotIndex, CBVs[Index]->GetRHI());
#else
		RHICmdList.SetShaderUniformBuffer(ComputeShaderRef, CBVResourceBindings[Index].SlotIndex, CBVs[Index]->GetRHI());
#endif
	}

	for (int32 Index = 0; Index < SRVs.Num(); Index++)
	{
		RHICmdList.Transition(SRVs[Index]->GetRHITransitionInfo());
#if COMPUSHADY_UE_VERSION >= 53
		BatchedParameters.SetShaderResourceViewParameter(SRVResourceBindings[Index].SlotIndex, SRVs[Index]->GetRHI());
#else
		RHICmdList.SetShaderResourceViewParameter(ComputeShaderRef, SRVResourceBindings[Index].SlotIndex, SRVs[Index]->GetRHI());
#endif
	}

	for (int32 Index = 0; Index < UAVs.Num(); Index++)
	{
		RHICmdList.Transition(UAVs[Index]->GetRHITransitionInfo());
#if COMPUSHADY_UE_VERSION >= 53
		BatchedParameters.SetUAVParameter(UAVResourceBindings[Index].SlotIndex, UAVs[Index]->GetRHI());
#else
		RHICmdList.SetUAVParameter(ComputeShaderRef, UAVResourceBindings[Index].SlotIndex, UAVs[Index]->GetRHI());
#endif
	}

#if COMPUSHADY_UE_VERSION >= 53
	RHICmdList.SetBatchedShaderParameters(ComputeShaderRef, BatchedParameters);
#endif
}

void UCompushadyCompute::Dispatch(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
{
	if (XYZ.X <= 0 || XYZ.Y <= 0 || XYZ.Z <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid ThreadGroupCount %s"), *XYZ.ToString()));
		return;
	}

	if (!SetupDispatch(ResourceArray, OnSignaled))
	{
		return;
	}

	const TArray<UCompushadyCBV*>& CBVs = ResourceArray.CBVs;
	const TArray<UCompushadySRV*>& SRVs = ResourceArray.SRVs;
	const TArray<UCompushadyUAV*>& UAVs = ResourceArray.UAVs;

	EnqueueToGPU(
		[this, ResourceArray, XYZ, CBVs, SRVs, UAVs](FRHICommandListImmediate& RHICmdList)
		{
			SetupPipeline(RHICmdList, CBVs, SRVs, UAVs);

			RHICmdList.DispatchComputeShader(XYZ.X, XYZ.Y, XYZ.Z);
		}, OnSignaled);
}

void UCompushadyCompute::DispatchByMap(const TMap<FString, UCompushadyResource*>& ResourceMap, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
{
	FCompushadyResourceArray ResourceArray;
	for (int32 Index = 0; Index < CBVResourceBindings.Num(); Index++)
	{
		const FString& Name = CBVResourceBindings[Index].Name;
		if (!ResourceMap.Contains(Name))
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Resource \"%s\" not found in supplied map"), *Name));
			return;
		}
		UCompushadyCBV* CBV = Cast<UCompushadyCBV>(ResourceMap[Name]);
		if (!CBV)
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected \"%s\" to be a CBV"), *Name));
			return;
		}
		ResourceArray.CBVs.Add(CBV);
	}

	for (int32 Index = 0; Index < SRVResourceBindings.Num(); Index++)
	{
		const FString& Name = SRVResourceBindings[Index].Name;
		if (!ResourceMap.Contains(Name))
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Resource \"%s\" not found in supplied map"), *Name));
			return;
		}
		UCompushadySRV* SRV = Cast<UCompushadySRV>(ResourceMap[Name]);
		if (!SRV)
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected \"%s\" to be a SRV"), *Name));
			return;
		}
		ResourceArray.SRVs.Add(SRV);
	}

	for (int32 Index = 0; Index < UAVResourceBindings.Num(); Index++)
	{
		const FString& Name = UAVResourceBindings[Index].Name;
		if (!ResourceMap.Contains(Name))
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Resource \"%s\" not found in supplied map"), *Name));
			return;
		}
		UCompushadyUAV* UAV = Cast<UCompushadyUAV>(ResourceMap[Name]);
		if (!UAV)
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected \"%s\" to be an UAV"), *Name));
			return;
		}
		ResourceArray.UAVs.Add(UAV);
	}

	UCompushadyCompute::Dispatch(ResourceArray, XYZ, OnSignaled);
}

void UCompushadyCompute::DispatchIndirect(const FCompushadyResourceArray& ResourceArray, UCompushadyResource* Buffer, const int32 Offset, const FCompushadySignaled& OnSignaled)
{
	if (!Buffer)
	{
		OnSignaled.ExecuteIfBound(false, "Buffer is NULL");
		return;
	}

	FBufferRHIRef BufferRHIRef = Buffer->GetBufferRHI();
	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
	{
		OnSignaled.ExecuteIfBound(false, "Invalid Indirect Buffer");
		return;
	}

	if (BufferRHIRef->GetSize() < (sizeof(uint32) * 3))
	{
		OnSignaled.ExecuteIfBound(false, "Invalid Indirect Buffer size (expected sizeof(uint32) * 3)");
		return;
	}

	if (!SetupDispatch(ResourceArray, OnSignaled))
	{
		return;
	}

	const TArray<UCompushadyCBV*>& CBVs = ResourceArray.CBVs;
	const TArray<UCompushadySRV*>& SRVs = ResourceArray.SRVs;
	const TArray<UCompushadyUAV*>& UAVs = ResourceArray.UAVs;

	EnqueueToGPU(
		[this, ResourceArray, BufferRHIRef, Offset, CBVs, SRVs, UAVs](FRHICommandListImmediate& RHICmdList)
		{
			SetupPipeline(RHICmdList, CBVs, SRVs, UAVs);

			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::IndirectArgs));
			RHICmdList.DispatchIndirectComputeShader(BufferRHIRef, Offset);
		}, OnSignaled);
}

bool UCompushadyCompute::IsRunning() const
{
	return ICompushadySignalable::IsRunning();
}

void UCompushadyCompute::OnSignalReceived()
{
	CurrentResourceArray.CBVs.Empty();
	CurrentResourceArray.SRVs.Empty();
	CurrentResourceArray.UAVs.Empty();
}

const TArray<uint8>& UCompushadyCompute::GetSPIRV() const
{
	return SPIRV;
}

const TArray<uint8>& UCompushadyCompute::GetDXIL() const
{
	return DXIL;
}

FIntVector UCompushadyCompute::GetThreadGroupSize() const
{
	return ThreadGroupSize;
}

void UCompushadyCompute::StoreLastSignal(bool bSuccess, const FString& ErrorMessage)
{
	bLastSuccess = bSuccess;
	LastErrorMessages = ErrorMessage;
}