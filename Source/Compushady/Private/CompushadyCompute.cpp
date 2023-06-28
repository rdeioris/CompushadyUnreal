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
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, "cs_6_0", ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	if (RHIInterfaceType == ERHIInterfaceType::Vulkan)
	{
		SPIRV = ByteCode;

		if (!FixupSPIRV(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
		{
			return false;
		}
	}
    else if (RHIInterfaceType == ERHIInterfaceType::Metal)
    {
        SPIRV = ByteCode;
        // TODO convert to MSL
        return false;
    }
	else if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		DXIL = ByteCode;

		if (!FixupDXIL(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
		{
			return false;
		}
	}

	return CreateComputePipeline(ByteCode, ShaderResourceBindings, ErrorMessages);
}

bool UCompushadyCompute::InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	bRunning = false;

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
	bRunning = false;

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
	bRunning = false;

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
    else if (RHIInterfaceType == ERHIInterfaceType::Metal)
    {
        // TODO: convert to MSL
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

bool UCompushadyCompute::SetupDispatch(const FCompushadyResourceArray& ResourceArray, const FCompushadySignaled& OnSignaled)
{
	if (bRunning)
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

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushady)(
		[this, ResourceArray, XYZ, CBVs, SRVs, UAVs](FRHICommandListImmediate& RHICmdList)
		{
			SetupPipeline(RHICmdList, CBVs, SRVs, UAVs);

	RHICmdList.DispatchComputeShader(XYZ.X, XYZ.Y, XYZ.Z);

	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
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

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushady)(
		[this, ResourceArray, BufferRHIRef, Offset, CBVs, SRVs, UAVs](FRHICommandListImmediate& RHICmdList)
		{
			SetupPipeline(RHICmdList, CBVs, SRVs, UAVs);

	RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::IndirectArgs));
	RHICmdList.DispatchIndirectComputeShader(BufferRHIRef, Offset);

	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

bool UCompushadyCompute::IsRunning() const
{
	return bRunning;
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
