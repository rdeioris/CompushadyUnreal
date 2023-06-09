// Copyright 2023 - Roberto De Ioris.


#include "CompushadyFunctionLibrary.h"
#include "CompushadyTypes.h"
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

UCompushadyCBV* UCompushadyFunctionLibrary::CreateCompushadyCBVFromFloatArray(const FString& Name, const TArray<float>& Data)
{
	UCompushadyCBV* CompushadyCBV = NewObject<UCompushadyCBV>();
	if (!CompushadyCBV->Initialize(Name, reinterpret_cast<const uint8*>(Data.GetData()), Data.Num() * sizeof(float)))
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

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromGLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();
	TArray<uint8> ShaderCode;
	if (!FFileHelper::LoadFileToArray(ShaderCode, *Filename))
	{
		return nullptr;
	}

	if (!CompushadyCompute->InitFromGLSL(ShaderCode, EntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromSPIRVFile(const FString& Filename, FString& ErrorMessages)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();
	TArray<uint8> ShaderCode;
	if (!FFileHelper::LoadFileToArray(ShaderCode, *Filename))
	{
		return nullptr;
	}

	if (!CompushadyCompute->InitFromSPIRV(ShaderCode, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromDXILFile(const FString& Filename, FString& ErrorMessages)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();
	TArray<uint8> ShaderCode;
	if (!FFileHelper::LoadFileToArray(ShaderCode, *Filename))
	{
		return nullptr;
	}

	if (!CompushadyCompute->InitFromDXIL(ShaderCode, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

bool UCompushadyFunctionLibrary::DisassembleSPIRVFile(const FString& Filename, FString& Disassembled, FString& ErrorMessages)
{
	TArray<uint8> ByteCode;
	if (!FFileHelper::LoadFileToArray(ByteCode, *Filename))
	{
		return false;
	}

	return Compushady::DisassembleSPIRV(ByteCode, Disassembled, ErrorMessages);
}

bool UCompushadyFunctionLibrary::DisassembleSPIRVBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages)
{
	return Compushady::DisassembleSPIRV(Blob, Disassembled, ErrorMessages);
}

bool UCompushadyFunctionLibrary::DisassembleDXILFile(const FString& Filename, FString& Disassembled, FString& ErrorMessages)
{
	TArray<uint8> ByteCode;
	if (!FFileHelper::LoadFileToArray(ByteCode, *Filename))
	{
		return false;
	}

	return Compushady::DisassembleDXIL(ByteCode, Disassembled, ErrorMessages);
}

bool UCompushadyFunctionLibrary::DisassembleDXILBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages)
{
	return Compushady::DisassembleDXIL(Blob, Disassembled, ErrorMessages);
}

bool UCompushadyFunctionLibrary::SPIRVBlobToHLSL(const TArray<uint8>& Blob, FString& HLSL, FString& ErrorMessages)
{
	return Compushady::SPIRVToHLSL(Blob, HLSL, ErrorMessages);
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

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLShaderAsset(UCompushadyShader* ShaderAsset, FString& ErrorMessages, const FString& EntryPoint)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();

	TStringBuilderBase<UTF8CHAR> SourceUTF8;
	SourceUTF8 = TCHAR_TO_UTF8(*ShaderAsset->Code);

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

	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
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

	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
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

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
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

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format)
{
	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create3D(*Name, Width, Height, Depth, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV);
	FTextureRHIRef TextureRHIRef = RHICreateTexture(TextureCreateDesc);

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format)
{
	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create3D(*Name, Width, Height, Depth, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource);
	FTextureRHIRef TextureRHIRef = RHICreateTexture(TextureCreateDesc);

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
	{
		return nullptr;
	}

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(TextureRHIRef))
	{
		return nullptr;
	}

	return CompushadySRV;
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromTextureCube(UTextureCube* TextureCube)
{
	if (TextureCube->IsStreamable() && !TextureCube->IsFullyStreamedIn())
	{
		TextureCube->SetForceMipLevelsToBeResident(30.0f);
		TextureCube->WaitForStreaming();
	}

	if (!TextureCube->GetResource() || !TextureCube->GetResource()->IsInitialized())
	{
		TextureCube->UpdateResource();
	}

	FlushRenderingCommands();

	FTextureResource* Resource = TextureCube->GetResource();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVTexture2DFromImageFile(const FString& Name, const FString& Filename)
{
	TArray<uint8> ImageData;
	if (!FFileHelper::LoadFileToArray(ImageData, *Filename))
	{
		return nullptr;
	}
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(ImageData.GetData(), ImageData.Num());
	if (ImageFormat == EImageFormat::Invalid)
	{
		return nullptr;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (!ImageWrapper.IsValid())
	{
		return nullptr;
	}

	if (!ImageWrapper->SetCompressed(ImageData.GetData(), ImageData.Num()))
	{
		return nullptr;
	}

	TArray<uint8> UncompressedBytes;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBytes))
	{
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), EPixelFormat::PF_B8G8R8A8);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource);
	FTextureRHIRef TextureRHIRef = RHICreateTexture(TextureCreateDesc);

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
	{
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(DoCompushadyUpdateTexture2D)(
		[TextureRHIRef, ImageWrapper, &UncompressedBytes](FRHICommandListImmediate& RHICmdList)
		{
			FUpdateTextureRegion2D UpdateTextureRegion2D(0, 0, 0, 0, ImageWrapper->GetWidth(), ImageWrapper->GetHeight());
	RHICmdList.UpdateTexture2D(TextureRHIRef, 0, UpdateTextureRegion2D, ImageWrapper->GetWidth() * 4, UncompressedBytes.GetData());
		});

	FlushRenderingCommands();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(TextureRHIRef))
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVBufferFromCurveFloat(const FString& Name, UCurveFloat* CurveFloat, const float StartTime, const float EndTime, const int32 Steps)
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

	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
	{
		return nullptr;
	}

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromBuffer(BufferRHIRef, EPixelFormat::PF_R32_FLOAT))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget)
{
	if (!RenderTarget)
	{
		return nullptr;
	}

	if (!RenderTarget->bCanCreateUAV || !RenderTarget->GetResource() || !RenderTarget->GetResource()->IsInitialized())
	{
		RenderTarget->bCanCreateUAV = true;
		RenderTarget->UpdateResource();
	}

	RenderTarget->UpdateResourceImmediate(false);
	FlushRenderingCommands();

	FTextureResource* Resource = RenderTarget->GetResource();

	UCompushadyUAV* CompushadyUAV = NewObject<UCompushadyUAV>();
	if (!CompushadyUAV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVFromRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray)
{
	if (!RenderTargetArray)
	{
		return nullptr;
	}

	if (!RenderTargetArray->bCanCreateUAV || !RenderTargetArray->GetResource() || !RenderTargetArray->GetResource()->IsInitialized())
	{
		RenderTargetArray->bCanCreateUAV = true;
		RenderTargetArray->UpdateResource();
	}

	RenderTargetArray->UpdateResourceImmediate(false);
	FlushRenderingCommands();

	FTextureResource* Resource = RenderTargetArray->GetResource();

	UCompushadyUAV* CompushadyUAV = NewObject<UCompushadyUAV>();
	if (!CompushadyUAV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVFromRenderTargetCube(UTextureRenderTargetCube* RenderTargetCube)
{
	if (!RenderTargetCube)
	{
		return nullptr;
	}

	if (!RenderTargetCube->bCanCreateUAV || !RenderTargetCube->GetResource() || !RenderTargetCube->GetResource()->IsInitialized())
	{
		RenderTargetCube->bCanCreateUAV = true;
		RenderTargetCube->UpdateResource();
	}

	RenderTargetCube->UpdateResourceImmediate(false);
	FlushRenderingCommands();

	FTextureResource* Resource = RenderTargetCube->GetResource();

	UCompushadyUAV* CompushadyUAV = NewObject<UCompushadyUAV>();
	if (!CompushadyUAV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVFromRenderTargetVolume(UTextureRenderTargetVolume* RenderTargetVolume)
{
	if (!RenderTargetVolume)
	{
		return nullptr;
	}

	if (!RenderTargetVolume->bCanCreateUAV || !RenderTargetVolume->GetResource() || !RenderTargetVolume->GetResource()->IsInitialized())
	{
		RenderTargetVolume->bCanCreateUAV = true;
		RenderTargetVolume->UpdateResource();
	}

	RenderTargetVolume->UpdateResourceImmediate(false);
	FlushRenderingCommands();

	FTextureResource* Resource = RenderTargetVolume->GetResource();

	UCompushadyUAV* CompushadyUAV = NewObject<UCompushadyUAV>();
	if (!CompushadyUAV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVFromStaticMeshPositionsCopy(const FString& Name, UStaticMesh* StaticMesh, const int32 LOD)
{
	if (!StaticMesh->AreRenderingResourcesInitialized())
	{
		StaticMesh->InitResources();
	}

	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();

	if (LOD >= RenderData->LODResources.Num())
	{
		return nullptr;
	}

	FStaticMeshLODResources& LODResources = RenderData->LODResources[LOD];

	FBufferRHIRef BufferRHIRef = LODResources.VertexBuffers.PositionVertexBuffer.VertexBufferRHI;
	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
	{
		return nullptr;
	}

	UCompushadyUAV* CompushadyUAV = CreateCompushadyUAVStructuredBuffer(Name, BufferRHIRef->GetSize(), sizeof(float) * 3);
	if (!CompushadyUAV)
	{
		return nullptr;
	}

	if (!CompushadyUAV->CopyFromRHIBuffer(BufferRHIRef))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVFromStaticMeshTexCoordsCopy(const FString& Name, UStaticMesh* StaticMesh, const int32 LOD)
{
	if (!StaticMesh->AreRenderingResourcesInitialized())
	{
		StaticMesh->InitResources();
	}

	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();

	if (LOD >= RenderData->LODResources.Num())
	{
		return nullptr;
	}

	FStaticMeshLODResources& LODResources = RenderData->LODResources[LOD];

	FBufferRHIRef BufferRHIRef = LODResources.VertexBuffers.StaticMeshVertexBuffer.TexCoordVertexBuffer.VertexBufferRHI;
	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
	{
		return nullptr;
	}

	UCompushadyUAV* CompushadyUAV = CreateCompushadyUAVStructuredBuffer(Name, BufferRHIRef->GetSize(), (LODResources.VertexBuffers.StaticMeshVertexBuffer.GetUseFullPrecisionUVs() ? sizeof(float) : 2) * 2 * LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords());
	if (!CompushadyUAV)
	{
		return nullptr;
	}

	if (!CompushadyUAV->CopyFromRHIBuffer(BufferRHIRef))
	{
		return nullptr;
	}

	return CompushadyUAV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const EPixelFormat PixelFormat)
{
	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
	BufferRHIRef = RHICreateBuffer(Data.Num() * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::UAVCompute, ResourceCreateInfo);
	void* LockedData = RHICmdList.LockBuffer(BufferRHIRef, 0, BufferRHIRef->GetSize(), EResourceLockMode::RLM_WriteOnly);
	FMemory::Memcpy(LockedData, Data.GetData(), BufferRHIRef->GetSize());
	RHICmdList.UnlockBuffer(BufferRHIRef);
		});

	FlushRenderingCommands();

	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
	{
		return nullptr;
	}

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromBuffer(BufferRHIRef, PixelFormat))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const int32 Stride)
{
	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, Stride](FRHICommandListImmediate& RHICmdList)
		{
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
	BufferRHIRef = RHICreateBuffer(Data.Num() * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::StructuredBuffer, Stride, ERHIAccess::UAVCompute, ResourceCreateInfo);
	void* LockedData = RHICmdList.LockBuffer(BufferRHIRef, 0, BufferRHIRef->GetSize(), EResourceLockMode::RLM_WriteOnly);
	FMemory::Memcpy(LockedData, Data.GetData(), BufferRHIRef->GetSize());
	RHICmdList.UnlockBuffer(BufferRHIRef);
		});

	FlushRenderingCommands();

	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
	{
		return nullptr;
	}

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromStructuredBuffer(BufferRHIRef))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySoundWave* UCompushadyFunctionLibrary::CreateCompushadySoundWave(const UCompushadyCompute* Compute, const FCompushadyResourceArray& ResourceArray, const float Duration)
{
	UCompushadySoundWave* CompushadySoundWave = NewObject<UCompushadySoundWave>();

	CompushadySoundWave->Duration = 10000;
	CompushadySoundWave->SetSampleRate(48000);
	CompushadySoundWave->NumChannels = 1;
	CompushadySoundWave->bLooping = true;

	return CompushadySoundWave;
}