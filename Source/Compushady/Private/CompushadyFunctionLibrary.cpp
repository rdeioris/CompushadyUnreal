// Copyright 2023 - Roberto De Ioris.


#include "CompushadyFunctionLibrary.h"
#include "Compushady.h"
#include "Engine/TextureRenderTarget2D.h"
#include "vulkan.h"
#include "VulkanCommon.h"
#include "VulkanShaderResources.h"
#include "Serialization/ArrayWriter.h"

UCompushadyCBV* UCompushadyFunctionLibrary::CreateCompushadyCBV(const FString& Name, const int64 Size)
{
	UCompushadyCBV* CompushadyCBV = NewObject<UCompushadyCBV>();
	if (!CompushadyCBV->Initialize(Name, nullptr, Size))
	{
		return nullptr;
	}

	return CompushadyCBV;
}

UCompushadyCBV* UCompushadyFunctionLibrary::CreateCompushadyCBVFromData(const FString& Name, const TArray<uint8>& Data)
{
	UCompushadyCBV* CompushadyCBV = NewObject<UCompushadyCBV>();
	if (!CompushadyCBV->Initialize(Name, Data.GetData(), Data.Num()))
	{
		return nullptr;
	}

	return CompushadyCBV;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();
	TArray<uint8> ShaderCode;
	if (!FFileHelper::LoadFileToArray(ShaderCode, *Filename))
	{
		return nullptr;
	}

	if (!CompushadyCompute->InitFromHLSL(ShaderCode, EntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(const FString& Source, FString& ErrorMessages, const FString& EntryPoint)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();

	TStringBuilderBase<UTF8CHAR> SourceUTF8;
	SourceUTF8 = TCHAR_TO_UTF8(*Source);

	TArray<uint8> ShaderCode;
	ShaderCode.Append(reinterpret_cast<const uint8*>(*SourceUTF8), SourceUTF8.Len());
	
	if (!CompushadyCompute->InitFromHLSL(ShaderCode, EntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(const FString& Name, const int64 Size, const EPixelFormat PixelFormat)
{
	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Size, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
	BufferRHIRef = RHICreateBuffer(Size, EBufferUsageFlags::ShaderResource | EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::UAVCompute, ResourceCreateInfo);
		});

	FlushRenderingCommands();

	if (!BufferRHIRef.IsValid())
	{
		return nullptr;
	}

	UCompushadyUAV* CompushadyUAV = NewObject<UCompushadyUAV>();
	if (!CompushadyUAV->InitializeFromBuffer(BufferRHIRef, PixelFormat))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVStructuredBuffer(const FString& Name, const int64 Size, const int32 Stride)
{
	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Size, Stride](FRHICommandListImmediate& RHICmdList)
		{
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
	BufferRHIRef = RHICreateBuffer(Size, EBufferUsageFlags::ShaderResource | EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::StructuredBuffer, Stride, ERHIAccess::UAVCompute, ResourceCreateInfo);
		});

	FlushRenderingCommands();

	if (!BufferRHIRef.IsValid())
	{
		return nullptr;
	}

	UCompushadyUAV* CompushadyUAV = NewObject<UCompushadyUAV>();
	if (!CompushadyUAV->InitializeFromStructuredBuffer(BufferRHIRef))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format)
{
	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV);
	FTextureRHIRef TextureRHIRef = RHICreateTexture(TextureCreateDesc);

	if (!TextureRHIRef.IsValid())
	{
		return nullptr;
	}

	UCompushadyUAV* CompushadyUAV = NewObject<UCompushadyUAV>();
	if (!CompushadyUAV->InitializeFromTexture(TextureRHIRef))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromTexture2D(UTexture2D* Texture2D)
{
	if (Texture2D->IsStreamable() && !Texture2D->IsFullyStreamedIn())
	{
		Texture2D->SetForceMipLevelsToBeResident(30.0f);
		Texture2D->WaitForStreaming();
	}

	if (!Texture2D->GetResource() || !Texture2D->GetResource()->IsInitialized())
	{
		Texture2D->UpdateResource();
	}

	FlushRenderingCommands();

	FTextureResource* Resource = Texture2D->GetResource();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget)
{
	if (!RenderTarget->GetResource() || !RenderTarget->GetResource()->IsInitialized())
	{
		RenderTarget->UpdateResource();
	}

	RenderTarget->UpdateResourceImmediate(false);
	FlushRenderingCommands();

	FTextureResource* Resource = RenderTarget->GetResource();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromCurveFloat(UCurveFloat* CurveFloat, const float StartTime, const float EndTime, const int32 Steps)
{
	if (!CurveFloat || Steps <= 0 || EndTime <= StartTime)
	{
		return nullptr;
	}

	const float Delta = (EndTime - StartTime) / Steps;
	float Time = StartTime;

	TArray<float> Data;

	for (int32 Step = 0; Step < Steps; Step++)
	{
		Data.Add(CurveFloat->GetFloatValue(Time));
		Time += Delta;
	}

	FBufferRHIRef BufferRHIRef;

	FString Name = CurveFloat->GetName();

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, Steps](FRHICommandListImmediate& RHICmdList)
		{
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
	BufferRHIRef = RHICreateBuffer(Steps * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, sizeof(float), ERHIAccess::UAVCompute, ResourceCreateInfo);
	void* LockedData = RHICmdList.LockBuffer(BufferRHIRef, 0, BufferRHIRef->GetSize(), EResourceLockMode::RLM_WriteOnly);
	FMemory::Memcpy(LockedData, Data.GetData(), BufferRHIRef->GetSize());
	RHICmdList.UnlockBuffer(BufferRHIRef);
		});

	FlushRenderingCommands();

	if (!BufferRHIRef.IsValid())
	{
		return nullptr;
	}

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromBuffer(BufferRHIRef, sizeof(float), EPixelFormat::PF_R32_FLOAT))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget)
{
	if (!RenderTarget->bCanCreateUAV || !RenderTarget->GetResource() || !RenderTarget->GetResource()->IsInitialized())
	{
		RenderTarget->bCanCreateUAV = true;
		RenderTarget->UpdateResource();
	}

	RenderTarget->UpdateResourceImmediate(false);
	FlushRenderingCommands();

	FTextureResource* Resource = RenderTarget->GetResource();
	UE_LOG(LogTemp, Error, TEXT("Resource: %p %p %d"), Resource, Resource->GetTextureRHI().GetReference(), Resource->IsInitialized());

	UCompushadyUAV* CompushadyUAV = NewObject<UCompushadyUAV>();
	if (!CompushadyUAV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadyUAV;

	//FUnorderedAccessViewRHIRef UAV = RHICreateUnorderedAccessView(Resource->GetTextureRHI().GetReference());
	//UE_LOG(LogTemp, Error, TEXT("UAV: %p"), UAV.GetReference());

	/*TArray<uint8> Data;
	TArray<uint8> ByteCode;
	TArray<uint8> Source;
	FFileHelper::LoadFileToArray(Source, TEXT("D:/Downloads/compushady_test.hlsl"));
	if (!Compushady::CompileHLSL(Source, "main", "cs_6_0", ByteCode))
	{
		return nullptr;
	}

	ERHIInterfaceType RHIInterfaceType = RHIGetInterfaceType();

	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		FShaderResourceTable ShaderResourceTable;
		FArrayWriter Writer;
		Writer << ShaderResourceTable;

		Data.Append(Writer);
	}
	else if (RHIInterfaceType == ERHIInterfaceType::Vulkan)
	{
		//"main_000006e4_00000000"
		FVulkanShaderHeader VulkanShaderHeader;
		VulkanShaderHeader.SpirvCRC = FCrc::MemCrc32(ByteCode.GetData(), ByteCode.Num());

		ANSICHAR EntryPoint[24];
		FCStringAnsi::Snprintf(EntryPoint, 24, "main_%0.8x_%0.8x", ByteCode.Num(), VulkanShaderHeader.SpirvCRC);

		UE_LOG(LogTemp, Error, TEXT("------------------------------- %s ----------------------------------"), ANSI_TO_TCHAR(EntryPoint));

		FVulkanShaderHeader::FSpirvInfo SpirvInfo;

		TArrayView<uint32> SpirV = TArrayView<uint32>((uint32*)ByteCode.GetData(), ByteCode.Num() / sizeof(uint32));

		int32 Offset = 5;

		while (Offset < SpirV.Num())
		{
			uint32 Word = SpirV[Offset];
			uint16 Opcode = Word & 0xFFFF;
			uint16 Size = Word >> 16;
			if (Size == 0)
			{
				break;
			}

			// patch the bindings/descriptor sets
			if (Opcode == 71 && (Offset + Size < SpirV.Num())) // OpDecorate(71) + id + Binding
			{
				if (Size > 3)
				{
					if (SpirV[Offset + 2] == 33) // Binding
					{
						UE_LOG(LogTemp, Warning, TEXT("Binding %u %u"), SpirV[Offset + 1], SpirV[Offset + 3]);
						SpirvInfo.BindingIndexOffset = Offset + 3;
					}
					if (SpirV[Offset + 2] == 34) // DescriptorSet
					{
						UE_LOG(LogTemp, Warning, TEXT("Descriptor Set %u %u"), SpirV[Offset + 1], SpirV[Offset + 3]);
						SpirvInfo.DescriptorSetOffset = Offset + 3;
					}
				}
			}
			// patch the EntryPoint Name
			else if (Opcode == 15 && (Offset + Size < SpirV.Num())) // OpEntryPoint(15) + ExecutionModel + id + Name
			{
				uint32* EntryPointPtr = reinterpret_cast<uint32*>(EntryPoint);
				for (int32 Index = 0; Index < 6; Index++)
				{
					SpirV[Offset + 3 + Index] = EntryPointPtr[Index];
				}
			}
			Offset += Size;
		}


		FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
		GlobalInfo.OriginalBindingIndex = 0;
		GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
		VulkanShaderHeader.Globals.Add(GlobalInfo);
		VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

		VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::StorageImage);

		FArrayWriter Writer;

		Writer << VulkanShaderHeader;

		int32 SpirvSize = ByteCode.Num();

		//Writer << SpirvSize;
		Writer << ByteCode;

		int32 MinusOne = -1;

		Writer << MinusOne;


		Data.Append(Writer);
	}
	else
	{
		return nullptr;
	}

	FShaderCode ShaderCode;
	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		ShaderCode.GetWriteAccess().Append(ByteCode);
	}

	FShaderCodePackedResourceCounts PackedResourceCounts = {};
	PackedResourceCounts.NumUAVs = 0;
	ShaderCode.AddOptionalData<FShaderCodePackedResourceCounts>(PackedResourceCounts);

	FShaderCodeResourceMasks ResourceMasks = {};
	ResourceMasks.UAVMask = 0xffffffff;
	ShaderCode.AddOptionalData<FShaderCodeResourceMasks>(ResourceMasks);

	Data.Append(ShaderCode.GetReadAccess());


	FSHA1 Sha1;
	Sha1.Update(Data.GetData(), Data.Num());
	FSHAHash Hash = Sha1.Finalize();

	FComputeShaderRHIRef ComputeShader = RHICreateComputeShader(Data, Hash);
	ComputeShader->SetHash(Hash);

	FComputePipelineStateRHIRef ComputePipelineState = RHICreateComputePipelineState(ComputeShader);

	FGPUFenceRHIRef Fence = RHICreateGPUFence(TEXT("CompushadyFence"));

	ENQUEUE_RENDER_COMMAND(DoCompushady)(
		[ComputeShader, Fence, UAV](FRHICommandListImmediate& RHICmdList)
		{

			SetComputePipelineState(RHICmdList, ComputeShader);


	RHICmdList.SetUAVParameter(ComputeShader, 0, UAV);

	RHICmdList.Transition(FRHITransitionInfo(UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	RHICmdList.DispatchComputeShader(128, 128, 1);

	RHICmdList.WriteGPUFence(Fence);
		});*/

	return nullptr;
}