// Copyright 2023 - Roberto De Ioris.


#include "CompushadyCompute.h"
#include "Compushady.h"
#include "Serialization/ArrayWriter.h"

bool UCompushadyCompute::InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	RHIInterfaceType = RHIGetInterfaceType();

	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, "cs_6_5", ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
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
	if (!Compushady::Utils::CreateResourceBindings(ShaderResourceBindings, ResourceBindings, ErrorMessages))
	{
		return false;
	}

	TArray<uint8> UnrealByteCode;
	FSHAHash Hash;
	if (!Compushady::ToUnrealShader(ByteCode, UnrealByteCode, ResourceBindings.NumCBVs, ResourceBindings.NumSRVs, ResourceBindings.NumUAVs, ResourceBindings.NumSamplers, Hash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the shader";
		return false;
	}

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

	return true;
}

void UCompushadyCompute::Dispatch(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
{
	if (XYZ.X <= 0 || XYZ.Y <= 0 || XYZ.Z <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid ThreadGroupCount %s"), *XYZ.ToString()));
		return;
	}

	if (!CheckResourceBindings(ResourceArray, ResourceBindings, OnSignaled))
	{
		return;
	}

	TrackResources(ResourceArray);

	EnqueueToGPU(
		[this, ResourceArray, XYZ](FRHICommandListImmediate& RHICmdList)
		{
			SetComputePipelineState(RHICmdList, ComputeShaderRef);
			Compushady::Utils::SetupPipelineParameters(RHICmdList, ComputeShaderRef, ResourceArray, ResourceBindings);

			RHICmdList.DispatchComputeShader(XYZ.X, XYZ.Y, XYZ.Z);
		}, OnSignaled);
}

void UCompushadyCompute::DispatchByMap(const TMap<FString, UCompushadyResource*>& ResourceMap, const FIntVector XYZ, const FCompushadySignaled& OnSignaled, const TMap<FString, UCompushadySampler*>& SamplerMap)
{
	FCompushadyResourceArray ResourceArray;

	for (int32 Index = 0; Index < ResourceBindings.CBVs.Num(); Index++)
	{
		const FString& Name = ResourceBindings.CBVs[Index].Name;
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

	for (int32 Index = 0; Index < ResourceBindings.SRVs.Num(); Index++)
	{
		const FString& Name = ResourceBindings.SRVs[Index].Name;
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

	for (int32 Index = 0; Index < ResourceBindings.UAVs.Num(); Index++)
	{
		const FString& Name = ResourceBindings.UAVs[Index].Name;
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

	for (int32 Index = 0; Index < ResourceBindings.Samplers.Num(); Index++)
	{
		const FString& Name = ResourceBindings.Samplers[Index].Name;
		if (!ResourceMap.Contains(Name))
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Sampler \"%s\" not found in supplied map"), *Name));
			return;
		}
		UCompushadySampler* Sampler = Cast<UCompushadySampler>(ResourceMap[Name]);
		if (!Sampler)
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Expected \"%s\" to be a Sampler"), *Name));
			return;
		}
		ResourceArray.Samplers.Add(Sampler);
	}

	Dispatch(ResourceArray, XYZ, OnSignaled);
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

	if (!CheckResourceBindings(ResourceArray, ResourceBindings, OnSignaled))
	{
		return;
	}

	TrackResources(ResourceArray);

	EnqueueToGPU(
		[this, ResourceArray, BufferRHIRef, Offset](FRHICommandListImmediate& RHICmdList)
		{
			SetComputePipelineState(RHICmdList, ComputeShaderRef);
			Compushady::Utils::SetupPipelineParameters(RHICmdList, ComputeShaderRef, ResourceArray, ResourceBindings);

			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::IndirectArgs));
			RHICmdList.DispatchIndirectComputeShader(BufferRHIRef, Offset);
		}, OnSignaled);
}

bool UCompushadyCompute::IsRunning() const
{
	return ICompushadySignalable::IsRunning();
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