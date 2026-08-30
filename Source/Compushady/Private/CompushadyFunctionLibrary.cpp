// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadyFunctionLibrary.h"
#include "CompushadyGPUFencePoller.h"
#include "Serialization/ArrayWriter.h"
#include "Misc/FileHelper.h"
#include "TextureRenderTargetVolumeResource.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/World.h"

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

UCompushadyCBV* UCompushadyFunctionLibrary::CreateCompushadyCBVFromIntArray(const FString& Name, const TArray<int32>& Data)
{
	UCompushadyCBV* CompushadyCBV = NewObject<UCompushadyCBV>();
	if (!CompushadyCBV->Initialize(Name, reinterpret_cast<const uint8*>(Data.GetData()), Data.Num() * sizeof(int32)))
	{
		return nullptr;
	}

	return CompushadyCBV;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint, const FCompushadyFileLoaderConfig& FileLoaderConfig)
{
	TArray<uint8> ShaderCode;
	if (!LoadFileWithLoaderConfig(Filename, ShaderCode, FileLoaderConfig))
	{
		return nullptr;
	}

	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();

	if (!CompushadyCompute->InitFromHLSL(ShaderCode, EntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromGLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint, const FCompushadyFileLoaderConfig& FileLoaderConfig)
{
	TArray<uint8> ShaderCode;
	if (!LoadFileWithLoaderConfig(Filename, ShaderCode, FileLoaderConfig))
	{
		return nullptr;
	}

	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();

	if (!CompushadyCompute->InitFromGLSL(ShaderCode, EntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromSPIRVFile(const FString& Filename, FString& ErrorMessages)
{
	TArray<uint8> ShaderCode;
	if (!FFileHelper::LoadFileToArray(ShaderCode, *Filename))
	{
		return nullptr;
	}

	return CreateCompushadyComputeFromSPIRVBlob(ShaderCode, ErrorMessages);
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromSPIRVBlob(const TArray<uint8>& Blob, FString& ErrorMessages)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();

	if (!CompushadyCompute->InitFromSPIRV(Blob, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyVideoEncoder* UCompushadyFunctionLibrary::CreateCompushadyVideoEncoder(const ECompushadyVideoEncoderCodec Codec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency)
{
	UCompushadyVideoEncoder* CompushadyVideoEncoder = NewObject<UCompushadyVideoEncoder>();

	if (!CompushadyVideoEncoder->Initialize(Codec, Quality, Latency))
	{
		return nullptr;
	}

	return CompushadyVideoEncoder;
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

	TArray<uint8> DisassembledBytes;
	if (!Compushady::DisassembleSPIRV(ByteCode, DisassembledBytes, ErrorMessages))
	{
		return false;
	}

	Disassembled = Compushady::ShaderCodeToString(DisassembledBytes);
	return true;
}

bool UCompushadyFunctionLibrary::DisassembleSPIRVBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages)
{
	TArray<uint8> DisassembledBytes;
	if (!Compushady::DisassembleSPIRV(Blob, DisassembledBytes, ErrorMessages))
	{
		return false;
	}

	Disassembled = Compushady::ShaderCodeToString(DisassembledBytes);
	return true;
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

bool UCompushadyFunctionLibrary::SPIRVBlobToHLSL(const TArray<uint8>& Blob, FString& HLSL, FString& EntryPoint, FString& ErrorMessages)
{
	TArray<uint8> HLSLBytes;
	if (!Compushady::SPIRVToHLSL(Blob, HLSLBytes, EntryPoint, ErrorMessages))
	{
		return false;
	}

	HLSL = Compushady::ShaderCodeToString(HLSLBytes);
	return true;
}

bool UCompushadyFunctionLibrary::CompileHLSLToSPIRVBlob(const FString& HLSL, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& Blob, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	TArray<uint8> HLSLShaderCode;
	Compushady::StringToShaderCode(HLSL, HLSLShaderCode);

	if (!Compushady::CompileHLSL(HLSLShaderCode, EntryPoint, TargetProfile, Blob, ErrorMessages, true))
	{
		return false;
	}

	// we need a copy as we want the original spirv bytecode
	TArray<uint8> BlobTempCopy = Blob;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;
	if (!Compushady::FixupSPIRV(BlobTempCopy, TargetProfile, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	return Compushady::Utils::CreateResourceBindings(ShaderResourceBindings, ResourceBindings, ErrorMessages);
}

bool UCompushadyFunctionLibrary::CompileGLSLToSPIRVBlob(const FString& HLSL, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& Blob, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	TArray<uint8> HLSLShaderCode;
	Compushady::StringToShaderCode(HLSL, HLSLShaderCode);

	if (!Compushady::CompileGLSL(HLSLShaderCode, EntryPoint, TargetProfile, Blob, ErrorMessages))
	{
		return false;
	}

	// we need a copy as we want the original spirv bytecode
	TArray<uint8> BlobTempCopy = Blob;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;
	if (!Compushady::FixupSPIRV(BlobTempCopy, TargetProfile, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	return Compushady::Utils::CreateResourceBindings(ShaderResourceBindings, ResourceBindings, ErrorMessages);
}

bool UCompushadyFunctionLibrary::SPIRVBlobToGLSL(const TArray<uint8>& Blob, FString& GLSL, FString& ErrorMessages)
{
	TArray<uint8> GLSLBytes;
	if (!Compushady::SPIRVToGLSL(Blob, GLSLBytes, ErrorMessages))
	{
		return false;
	}

	GLSL = Compushady::ShaderCodeToString(GLSLBytes);
	return true;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(const FString& ShaderSource, FString& ErrorMessages, const FString& EntryPoint)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(ShaderSource, ShaderCode);

	if (!CompushadyCompute->InitFromHLSL(ShaderCode, EntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromGLSLString(const FString& ShaderSource, FString& ErrorMessages, const FString& EntryPoint)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(ShaderSource, ShaderCode);

	if (!CompushadyCompute->InitFromGLSL(ShaderCode, EntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadyRasterizer* UCompushadyFunctionLibrary::CreateCompushadyVSPSRasterizerFromHLSLString(const FString& VertexShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& VertexShaderEntryPoint, const FString& PixelShaderEntryPoint)
{
	UCompushadyRasterizer* CompushadyRasterizer = NewObject<UCompushadyRasterizer>();

	TArray<uint8> VertexShaderCode;
	Compushady::StringToShaderCode(VertexShaderSource, VertexShaderCode);

	TArray<uint8> PixelShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, PixelShaderCode);

	if (!CompushadyRasterizer->InitVSPSFromHLSL(VertexShaderCode, VertexShaderEntryPoint, PixelShaderCode, PixelShaderEntryPoint, RasterizerConfig, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyRasterizer;
}

UCompushadyRasterizer* UCompushadyFunctionLibrary::CreateCompushadyMSPSRasterizerFromHLSLString(const FString& MeshShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& MeshShaderEntryPoint, const FString& PixelShaderEntryPoint)
{
	UCompushadyRasterizer* CompushadyRasterizer = NewObject<UCompushadyRasterizer>();

	TArray<uint8> MeshShaderCode;
	Compushady::StringToShaderCode(MeshShaderSource, MeshShaderCode);

	TArray<uint8> PixelShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, PixelShaderCode);

	if (!CompushadyRasterizer->InitMSPSFromHLSL(MeshShaderCode, MeshShaderEntryPoint, PixelShaderCode, PixelShaderEntryPoint, RasterizerConfig, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyRasterizer;
}

UCompushadyCompute* UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLShaderAsset(UCompushadyShader* ShaderAsset, FString& ErrorMessages, const FString& EntryPoint)
{
	UCompushadyCompute* CompushadyCompute = NewObject<UCompushadyCompute>();

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(ShaderAsset->Code, ShaderCode);

	if (!CompushadyCompute->InitFromHLSL(ShaderCode, EntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyCompute;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVBuffer(const FString& Name, const int64 Size, const EPixelFormat PixelFormat)
{
	if (Size <= 0)
	{
		return nullptr;
	}

	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Size, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Size, EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::SRVMask);
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

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(const FString& Name, const int64 Size, const EPixelFormat PixelFormat)
{
	if (Size <= 0)
	{
		return nullptr;
	}

	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Size, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Size, EBufferUsageFlags::ShaderResource | EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::UAVMask);
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
	if (Size <= 0 || Stride <= 0)
	{
		return nullptr;
	}

	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Size, Stride](FRHICommandListImmediate& RHICmdList)
		{
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Size, EBufferUsageFlags::ShaderResource | EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::StructuredBuffer, Stride, ERHIAccess::UAVCompute);
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
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV);

	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

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

UCompushadyRTV* UCompushadyFunctionLibrary::CreateCompushadyRTVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format, const FLinearColor ClearColor)
{
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format);
	TextureCreateDesc.ClearValue = FClearValueBinding(ClearColor);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
	{
		return nullptr;
	}

	UCompushadyRTV* CompushadyRTV = NewObject<UCompushadyRTV>();
	if (!CompushadyRTV->InitializeFromTexture(TextureRHIRef))
	{
		return nullptr;
	}

	return CompushadyRTV;
}

UCompushadyDSV* UCompushadyFunctionLibrary::CreateCompushadyDSVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format, const float DepthClearValue, const int32 StencilClearValue)
{
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	if (StencilClearValue < 0)
	{
		return nullptr;
	}

	if (Format != EPixelFormat::PF_D24 && Format != EPixelFormat::PF_ShadowDepth && Format != EPixelFormat::PF_DepthStencil)
	{
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format);
	TextureCreateDesc.ClearValue = FClearValueBinding(DepthClearValue, static_cast<uint32>(StencilClearValue));
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::DepthStencilTargetable);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
	{
		return nullptr;
	}

	UCompushadyDSV* CompushadyDSV = NewObject<UCompushadyDSV>();
	if (!CompushadyDSV->InitializeFromTexture(TextureRHIRef))
	{
		return nullptr;
	}

	return CompushadyDSV;
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format)
{
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create3D(*Name, Width, Height, Depth, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

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

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVTexture2DArray(const FString& Name, const int32 Width, const int32 Height, const int32 Slices, const EPixelFormat Format)
{
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2DArray(*Name, Width, Height, Slices, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

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

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVTextureCube(const FString& Name, const int32 Width, const EPixelFormat Format)
{
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::CreateCube(*Name, Width, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

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
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create3D(*Name, Width, Height, Depth, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format)
{
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromTexture2DArray(UTexture2DArray* Texture2DArray)
{
	if (Texture2DArray->IsStreamable() && !Texture2DArray->IsFullyStreamedIn())
	{
		Texture2DArray->SetForceMipLevelsToBeResident(30.0f);
		Texture2DArray->WaitForStreaming();
	}

	if (!Texture2DArray->GetResource() || !Texture2DArray->GetResource()->IsInitialized())
	{
		Texture2DArray->UpdateResource();
	}

	FlushRenderingCommands();

	FTextureResource* Resource = Texture2DArray->GetResource();

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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVTexture2DFromRandomStream(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format, const FRandomStream& RandomStream)
{
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	if (Width <= 0 || Height <= 0)
	{
		return nullptr;
	}

	TArray<uint8> ImageData;
	ImageData.AddUninitialized(Width * Height * GPixelFormats[Format].BlockBytes);
	for (int32 ByteIndex = 0; ByteIndex < ImageData.Num(); ByteIndex++)
	{
		ImageData[ByteIndex] = static_cast<uint8>(RandomStream.RandRange(0, 255));
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
	{
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(DoCompushadyUpdateTexture2D)(
		[TextureRHIRef, Width, Height, Format, &ImageData](FRHICommandListImmediate& RHICmdList)
		{
			FUpdateTextureRegion2D UpdateTextureRegion2D(0, 0, 0, 0, Width, Height);
			RHICmdList.UpdateTexture2D(TextureRHIRef, 0, UpdateTextureRegion2D, Width * GPixelFormats[Format].BlockBytes, ImageData.GetData());
		});

	FlushRenderingCommands();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(TextureRHIRef))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVTexture3DFromFile(const FString& Name, const FString& Filename, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format, const int64 Offset)
{
	if (!GPixelFormats[Format].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported Texture Format %s"), GetPixelFormatString(Format));
		return nullptr;
	}

	if (Width <= 0 || Height <= 0 || Depth <= 0)
	{
		return nullptr;
	}

	TArray64<uint8> SlicesData;
	if (!FFileHelper::LoadFileToArray(SlicesData, *Filename))
	{
		return nullptr;
	}

	const int64 RequiredSize = Width * Height * Depth * GPixelFormats[Format].BlockBytes;

	if (SlicesData.Num() - Offset < RequiredSize)
	{
		return nullptr;
	}

	FTextureRHIRef TextureRHIRef = nullptr;

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create3D(*Name, Width, Height, Depth, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource);

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
	{
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(DoCompushadyUpdateTexture3D)(
		[TextureRHIRef, &SlicesData, Offset, Width, Height, Depth, Format](FRHICommandListImmediate& RHICmdList)
		{
			FUpdateTextureRegion3D UpdateTextureRegion3D(0, 0, 0, 0, 0, 0, Width, Height, Depth);
			RHICmdList.UpdateTexture3D(TextureRHIRef, 0, UpdateTextureRegion3D, Width * GPixelFormats[Format].BlockBytes, Width * Height * GPixelFormats[Format].BlockBytes, SlicesData.GetData() + Offset);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromMediaTexture(UMediaTexture* MediaTexture)
{
	if (!MediaTexture->GetResource() || !MediaTexture->GetResource()->IsInitialized())
	{
		MediaTexture->UpdateResource();
	}

	FlushRenderingCommands();

	FTextureResource* Resource = MediaTexture->GetResource();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadyRTV* UCompushadyFunctionLibrary::CreateCompushadyRTVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget)
{
	if (!RenderTarget->GetResource() || !RenderTarget->GetResource()->IsInitialized())
	{
		RenderTarget->UpdateResource();
	}

	RenderTarget->UpdateResourceImmediate(false);
	FlushRenderingCommands();

	FTextureResource* Resource = RenderTarget->GetResource();

	UCompushadyRTV* CompushadyRTV = NewObject<UCompushadyRTV>();
	if (!CompushadyRTV->InitializeFromTexture(Resource->GetTextureRHI()))
	{
		return nullptr;
	}

	return CompushadyRTV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVBufferFromDataTable(const FString& Name, UDataTable* DataTable, const EPixelFormat PixelFormat)
{
	if (!DataTable)
	{
		return nullptr;
	}

	int64 Size = 0;
	TArray<FProperty*> Columns;

	// compute size of the buffer
	const UScriptStruct* ScriptStruct = DataTable->GetRowStruct();
	for (TFieldIterator<FProperty> It(ScriptStruct); It; ++It)
	{
		FProperty* Property = *It;

		if (CastField<FFloatProperty>(Property))
		{
			Size += sizeof(float);
			Columns.Add(Property);
		}
	}

	const TMap<FName, uint8*> RowMap = DataTable->GetRowMap();

	Size *= RowMap.Num();

	TArray<uint8> Data;
	Data.AddUninitialized(Size);

	int64 Offset = 0;

	for (const TPair<FName, uint8*>& Pair : RowMap)
	{
		for (const FProperty* Property : Columns)
		{
			float* Value = Property->ContainerPtrToValuePtr<float>(Pair.Value);
			FMemory::Memcpy(Data.GetData() + Offset, Value, sizeof(float));
			Offset += sizeof(float);
		}
	}

	return CreateCompushadySRVBufferFromByteArray(Name, Data, PixelFormat);
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
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Steps * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, sizeof(float), ERHIAccess::UAVCompute);
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

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVTextureRenderTarget2D(const int32 Width, const int32 Height, const EPixelFormat Format, UTextureRenderTarget2D*& RenderTarget, const FLinearColor ClearColor, const float Gamma, const bool bLinearGamma)
{
	RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->TargetGamma = Gamma;
	RenderTarget->ClearColor = ClearColor;
	RenderTarget->bCanCreateUAV = true;
	RenderTarget->InitCustomFormat(Width, Height, Format, bLinearGamma);

	return CreateCompushadyUAVFromRenderTarget2D(RenderTarget);
}

UCompushadyRTV* UCompushadyFunctionLibrary::CreateCompushadyRTVTextureRenderTarget2D(const int32 Width, const int32 Height, const EPixelFormat Format, UTextureRenderTarget2D*& RenderTarget, const FLinearColor ClearColor, const float Gamma, const bool bLinearGamma)
{
	RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->TargetGamma = Gamma;
	RenderTarget->ClearColor = ClearColor;
	RenderTarget->bCanCreateUAV = true;
	RenderTarget->InitCustomFormat(Width, Height, Format, bLinearGamma);

	return CreateCompushadyRTVFromRenderTarget2D(RenderTarget);
}

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVTextureRenderTargetCube(const int32 Width, const EPixelFormat Format, UTextureRenderTargetCube*& RenderTarget, const FLinearColor ClearColor, const float Gamma, const bool bLinearGamma)
{
	RenderTarget = NewObject<UTextureRenderTargetCube>();
	RenderTarget->TargetGamma = Gamma;
	RenderTarget->ClearColor = ClearColor;
	RenderTarget->bCanCreateUAV = true;
	RenderTarget->bForceLinearGamma = bLinearGamma;
	RenderTarget->Init(Width, Format);

	return CreateCompushadyUAVFromRenderTargetCube(RenderTarget);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const EPixelFormat PixelFormat)
{
	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Data.Num() * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::SRVMask);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVBufferFromByteArray(const FString& Name, const TArray<uint8>& Data, const EPixelFormat PixelFormat)
{
	if (PixelFormat == EPixelFormat::PF_Unknown || Data.Num() == 0)
	{
		return nullptr;
	}

	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Data.Num(), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::SRVMask);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVBufferFromFile(const FString& Name, const FString& Filename, const EPixelFormat PixelFormat)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}

	return CreateCompushadySRVBufferFromByteArray(Name, Data, PixelFormat);
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const int32 Stride)
{
	if (Data.Num() == 0)
	{
		return nullptr;
	}

	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, Stride](FRHICommandListImmediate& RHICmdList)
		{
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Data.Num() * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::StructuredBuffer, Stride, ERHIAccess::SRVMask);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromByteArray(const FString& Name, const TArray<uint8>& Data, const int32 Stride, const int64 Offset)
{
	if (Offset >= Data.Num())
	{
		return nullptr;
	}

	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, Stride, Offset](FRHICommandListImmediate& RHICmdList)
		{
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(*Name, Data.Num() - Offset, EBufferUsageFlags::ShaderResource | EBufferUsageFlags::StructuredBuffer, Stride, ERHIAccess::SRVMask);
			void* LockedData = RHICmdList.LockBuffer(BufferRHIRef, 0, BufferRHIRef->GetSize(), EResourceLockMode::RLM_WriteOnly);
			FMemory::Memcpy(LockedData, Data.GetData() + Offset, BufferRHIRef->GetSize() - Offset);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromFile(const FString& Name, const FString& Filename, const int32 Stride, const int64 Offset)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}
	return CreateCompushadySRVStructuredBufferFromByteArray(Name, Data, Stride, Offset);
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromStringArray(const FString& Name, const TArray<FString>& Lines, const TArray<int32>& Columns, const FString& Separator, const int32 SkipLines, const bool bCullEmpty)
{
	TArray<float> Values;
	int32 Stride;
	if (!Compushady::Utils::SplitToFloats(Lines, Columns, Separator, SkipLines, bCullEmpty, Values, Stride))
	{
		return nullptr;
	}

	return CreateCompushadySRVStructuredBufferFromFloatArray(Name, Values, Stride);
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromASCIIFile(const FString& Name, const FString& Filename, const TArray<int32>& Columns, const FString& Separator, const int32 SkipLines, const bool bCullEmpty)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}

	TArray<FString> Lines;
	Compushady::Utils::BytesToStringArray(Data, Lines);

	return CreateCompushadySRVStructuredBufferFromStringArray(Name, Lines, Columns, Separator, SkipLines, bCullEmpty);
}

UCompushadySampler* UCompushadyFunctionLibrary::CreateCompushadySampler(const TextureFilter Filter, const ECompushadySamplerAddressMode AddressU, const ECompushadySamplerAddressMode AddressV, const ECompushadySamplerAddressMode AddressW, const FLinearColor BorderColor)
{
	UCompushadySampler* CompushadySampler = NewObject<UCompushadySampler>();

	ESamplerFilter SamplerFilter = ESamplerFilter::SF_Point;
	if (Filter == TextureFilter::TF_Bilinear)
	{
		SamplerFilter = ESamplerFilter::SF_Bilinear;
	}
	else if (Filter == TextureFilter::TF_Trilinear)
	{
		SamplerFilter = ESamplerFilter::SF_Trilinear;
	}

	auto GetAddressMode = [](const ECompushadySamplerAddressMode Address) -> ESamplerAddressMode
		{
			switch (Address)
			{
			case(ECompushadySamplerAddressMode::Clamp):
				return ESamplerAddressMode::AM_Clamp;
				break;
			case(ECompushadySamplerAddressMode::Mirror):
				return ESamplerAddressMode::AM_Mirror;
				break;
			case(ECompushadySamplerAddressMode::Border):
				return ESamplerAddressMode::AM_Border;
				break;
			default:
				return ESamplerAddressMode::AM_Wrap;
				break;
			}
		};

	FSamplerStateInitializerRHI SamplerStateInitializer(SamplerFilter, GetAddressMode(AddressU), GetAddressMode(AddressV), GetAddressMode(AddressW));
	SamplerStateInitializer.BorderColor = BorderColor.ToFColor(false).R;
	FSamplerStateRHIRef SamplerState = RHICreateSamplerState(SamplerStateInitializer);
	if (!SamplerState.IsValid() || !SamplerState->IsValid())
	{
		return nullptr;
	}

	if (!CompushadySampler->InitializeFromSamplerState(SamplerState))
	{
		return nullptr;
	}

	return CompushadySampler;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromStaticMeshPositionBuffer(UStaticMesh* StaticMesh, int32& NumVertices, const int32 LOD)
{
	if (!StaticMesh)
	{
		return nullptr;
	}

	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
	if (!RenderData)
	{
		StaticMesh->InitResources();
		FlushRenderingCommands();
	}

	RenderData = StaticMesh->GetRenderData();
	if (!RenderData)
	{
		UE_LOG(LogCompushady, Error, TEXT("No RenderData for StaticMesh"));
		return nullptr;
	}

	if (!RenderData->LODResources.IsValidIndex(LOD))
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid LOD index for StaticMesh"));
		return nullptr;
	}

	const FStaticMeshLODResources& Resources = RenderData->LODResources[LOD];

	if (!Resources.VertexBuffers.PositionVertexBuffer.IsInitialized())
	{
		UE_LOG(LogCompushady, Error, TEXT("PositionVertexBuffer is not initialized"));
		return nullptr;
	}

	NumVertices = Resources.VertexBuffers.PositionVertexBuffer.GetNumVertices();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromBuffer(Resources.VertexBuffers.PositionVertexBuffer.VertexBufferRHI, EPixelFormat::PF_R32_FLOAT))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromStaticMeshUVBuffer(UStaticMesh* StaticMesh, int32& NumVertices, int32& NumUVs, const int32 LOD)
{
	if (!StaticMesh)
	{
		return nullptr;
	}

	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
	if (!RenderData)
	{
		StaticMesh->InitResources();
		FlushRenderingCommands();
	}

	RenderData = StaticMesh->GetRenderData();
	if (!RenderData)
	{
		UE_LOG(LogCompushady, Error, TEXT("No RenderData for StaticMesh"));
		return nullptr;
	}

	if (!RenderData->LODResources.IsValidIndex(LOD))
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid LOD index for StaticMesh"));
		return nullptr;
	}

	const FStaticMeshLODResources& Resources = RenderData->LODResources[LOD];

	if (!Resources.VertexBuffers.StaticMeshVertexBuffer.TexCoordVertexBuffer.IsInitialized())
	{
		UE_LOG(LogCompushady, Error, TEXT("TexCoordVertexBuffer is not initialized"));
		return nullptr;
	}

	NumVertices = Resources.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices();

	NumUVs = Resources.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromBuffer(Resources.VertexBuffers.StaticMeshVertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, Resources.VertexBuffers.StaticMeshVertexBuffer.GetUseFullPrecisionUVs() ? EPixelFormat::PF_R32_FLOAT : EPixelFormat::PF_R16F))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromStaticMeshTangentBuffer(UStaticMesh* StaticMesh, int32& NumTangents, const int32 LOD)
{
	if (!StaticMesh)
	{
		return nullptr;
	}

	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
	if (!RenderData)
	{
		StaticMesh->InitResources();
		FlushRenderingCommands();
	}

	RenderData = StaticMesh->GetRenderData();
	if (!RenderData)
	{
		UE_LOG(LogCompushady, Error, TEXT("No RenderData for StaticMesh"));
		return nullptr;
	}

	if (!RenderData->LODResources.IsValidIndex(LOD))
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid LOD index for StaticMesh"));
		return nullptr;
	}

	const FStaticMeshLODResources& Resources = RenderData->LODResources[LOD];

	if (!Resources.VertexBuffers.StaticMeshVertexBuffer.TangentsVertexBuffer.IsInitialized())
	{
		UE_LOG(LogCompushady, Error, TEXT("TangentsVertexBuffer is not initialized"));
		return nullptr;
	}

	const EPixelFormat PixelFormat = Resources.VertexBuffers.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis() ? PF_R16G16B16A16_SNORM : PF_R8G8B8A8_SNORM;

	NumTangents = Resources.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromBuffer(Resources.VertexBuffers.StaticMeshVertexBuffer.TangentsVertexBuffer.VertexBufferRHI, PixelFormat))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromStaticMeshIndexBuffer(UStaticMesh* StaticMesh, int32& NumIndices, const int32 LOD)
{
	if (!StaticMesh)
	{
		return nullptr;
	}

	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
	if (!RenderData)
	{
		StaticMesh->InitResources();
		FlushRenderingCommands();
	}

	RenderData = StaticMesh->GetRenderData();
	if (!RenderData)
	{
		UE_LOG(LogCompushady, Error, TEXT("No RenderData for StaticMesh"));
		return nullptr;
	}

	if (!RenderData->LODResources.IsValidIndex(LOD))
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid LOD index for StaticMesh"));
		return nullptr;
	}

	const FStaticMeshLODResources& Resources = RenderData->LODResources[LOD];

	if (!Resources.IndexBuffer.IsInitialized())
	{
		UE_LOG(LogCompushady, Error, TEXT("IndexBuffer is not initialized"));
		return nullptr;
	}

	EPixelFormat PixelFormat = EPixelFormat::PF_R16_UINT;
	if (Resources.IndexBuffer.Is32Bit())
	{
		PixelFormat = EPixelFormat::PF_R32_UINT;
	}

	NumIndices = Resources.IndexBuffer.GetNumIndices();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromBuffer(Resources.IndexBuffer.IndexBufferRHI, PixelFormat))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromSceneTexture(const ECompushadySceneTexture SceneTexture)
{
	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromSceneTexture(SceneTexture))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromResource(UCompushadyResource* Resource, const int32 Slice, const int32 MipLevel, const int32 NumSlices, const int32 NumMips, const EPixelFormat PixelFormat)
{
	if (!Resource)
	{
		return nullptr;
	}

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();

	if (Resource->IsValidBuffer())
	{
		if (EnumHasAnyFlags(Resource->GetBufferRHI()->GetUsage(), EBufferUsageFlags::StructuredBuffer))
		{
			if (!CompushadySRV->InitializeFromStructuredBuffer(Resource->GetBufferRHI()))
			{
				return nullptr;
			}
		}
		else
		{
			if (!CompushadySRV->InitializeFromBuffer(Resource->GetBufferRHI(), PixelFormat))
			{
				return nullptr;
			}
		}
	}
	else if (Resource->IsValidTexture())
	{
		if (!CompushadySRV->InitializeFromTextureAdvanced(Resource->GetTextureRHI(), Slice, NumSlices, MipLevel, NumMips, PixelFormat))
		{
			return nullptr;
		}
	}
	else
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromWorldSceneAccelerationStructure(UObject* WorldContextObject)
{
	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromWorldSceneAccelerationStructure(WorldContextObject->GetWorld()))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadyRayTracer* UCompushadyFunctionLibrary::CreateCompushadyRayTracerFromHLSLString(const FString& RayGenShaderSource, const FString& RayHitShaderSource, const FString& RayMissShaderSource, FString& ErrorMessages, const FString& RayGenShaderEntryPoint, const FString& RayHitShaderEntryPoint, const FString& RayMissShaderEntryPoint)
{
	UCompushadyRayTracer* CompushadyRayTracer = NewObject<UCompushadyRayTracer>();

	TArray<uint8> RayGenShaderCode;
	Compushady::StringToShaderCode(RayGenShaderSource, RayGenShaderCode);

	TArray<uint8> RayHitShaderCode;
	Compushady::StringToShaderCode(RayHitShaderSource, RayHitShaderCode);

	TArray<uint8> RayMissShaderCode;
	Compushady::StringToShaderCode(RayMissShaderSource, RayMissShaderCode);

	if (!CompushadyRayTracer->InitFromHLSL(RayGenShaderCode, RayGenShaderEntryPoint, RayHitShaderCode, RayHitShaderEntryPoint, RayMissShaderCode, RayMissShaderEntryPoint, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyRayTracer;
}

UCompushadyRasterizer* UCompushadyFunctionLibrary::CreateCompushadyVSPSRasterizerFromGLSLString(const FString& VertexShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& VertexShaderEntryPoint, const FString& PixelShaderEntryPoint)
{
	UCompushadyRasterizer* CompushadyRasterizer = NewObject<UCompushadyRasterizer>();

	TArray<uint8> VertexShaderCode;
	Compushady::StringToShaderCode(VertexShaderSource, VertexShaderCode);

	TArray<uint8> PixelShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, PixelShaderCode);

	if (!CompushadyRasterizer->InitVSPSFromGLSL(VertexShaderCode, VertexShaderEntryPoint, PixelShaderCode, PixelShaderEntryPoint, RasterizerConfig, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyRasterizer;
}

FCompushadyFloat UCompushadyFunctionLibrary::Conv_DoubleToCompushadyFloat(double Value)
{
	FCompushadyFloat CompushadyFloat;
	CompushadyFloat.Value = Value;
	return CompushadyFloat;
}

void UCompushadyFunctionLibrary::EnqueueToGPUMulti(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction, const FCompushadySignaled& OnSignaled, TArray<ICompushadyPipeline*> Pipelines)
{

	// Single fence written after the whole multi-pass batch has been recorded; the poller
	// fires the completion once the GPU has actually finished every pass, instead of firing
	// as soon as the render thread was done recording (the previous empty-task behaviour).
	FGPUFenceRHIRef SignalFence = RHICreateGPUFence(TEXT("CompushadyEnqueueToGPUMulti"));

	ENQUEUE_RENDER_COMMAND(DoCompushadyEnqueueToGPUMulti)(
		[InFunction, SignalFence](FRHICommandListImmediate& RHICmdList)
		{
			InFunction(RHICmdList);
			RHICmdList.WriteGPUFence(SignalFence.GetReference());
		});

	FCompushadyGPUFencePoller::Get().RegisterFence(SignalFence, [OnSignaled, Pipelines]()
		{
			for (ICompushadyPipeline* Pipeline : Pipelines)
			{
				Pipeline->UntrackResourcesAndUnmarkAsRunning();
			}
			OnSignaled.ExecuteIfBound(true, "");
		});
}

void UCompushadyFunctionLibrary::DispatchMultiPass(const TArray<FCompushadyComputePass>& ComputePasses, const FCompushadySignaled& OnSignaled)
{
	TArray<UCompushadyCompute*> ComputesArray;
	TArray<FCompushadyResourceArray> ResourceArrays;
	TArray<FIntVector> XYZs;

	for (const FCompushadyComputePass& ComputePass : ComputePasses)
	{
		if (!ComputePass.Compute)
		{
			OnSignaled.ExecuteIfBound(false, TEXT("Compute is NULL"));
			return;
		}

		if (ComputePass.Compute->IsRunning())
		{
			OnSignaled.ExecuteIfBound(false, "The Compute is already running");
			return;
		}

		if (ComputePass.XYZ.X <= 0 || ComputePass.XYZ.Y <= 0 || ComputePass.XYZ.Z <= 0)
		{
			OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid ThreadGroupCount %s"), *ComputePass.XYZ.ToString()));
			return;
		}

		FString ErrorMessages;
		if (!Compushady::Utils::ValidateResourceBindings(ComputePass.ResourceArray, ComputePass.Compute->ResourceBindings, ErrorMessages))
		{
			OnSignaled.ExecuteIfBound(false, ErrorMessages);
			return;
		}

		ComputesArray.Add(ComputePass.Compute);
		ResourceArrays.Add(ComputePass.ResourceArray);
		XYZs.Add(ComputePass.XYZ);
	}

	for (int32 Index = 0; Index < ComputesArray.Num(); Index++)
	{
		ComputesArray[Index]->TrackResourcesAndMarkAsRunning(ResourceArrays[Index]);
	}

	EnqueueToGPUMulti(
		[ComputesArray, ResourceArrays, XYZs](FRHICommandListImmediate& RHICmdList)
		{
			for (int32 Index = 0; Index < ComputesArray.Num(); Index++)
			{
				ComputesArray[Index]->Dispatch_RenderThread(RHICmdList, ResourceArrays[Index], XYZs[Index]);
			}
		}, OnSignaled, static_cast<TArray<ICompushadyPipeline*>>(ComputesArray));
}

bool UCompushadyFunctionLibrary::LoadFileWithLoaderConfig(const FString& Filename, TArray<uint8>& Bytes, const FCompushadyFileLoaderConfig& FileLoaderConfig)
{
	for (const FString& PrependFilename : FileLoaderConfig.PrependFiles)
	{
		TArray<uint8> Data;
		if (!Compushady::FileToByteArray(PrependFilename, FileLoaderConfig.bRelativeToContent, Data))
		{
			return false;
		}

		Bytes.Append(MoveTemp(Data));
	}

	for (const FString& PrependString : FileLoaderConfig.PrependStrings)
	{
		TArray<uint8> Data;
		Compushady::StringToShaderCode(PrependString, Data);

		Bytes.Append(MoveTemp(Data));
	}

	TArray<uint8> MainData;
	if (!Compushady::FileToByteArray(Filename, FileLoaderConfig.bRelativeToContent, MainData))
	{
		return false;
	}

	Bytes.Append(MoveTemp(MainData));

	for (const FString& AppendFilename : FileLoaderConfig.AppendFiles)
	{
		TArray<uint8> Data;
		if (!Compushady::FileToByteArray(AppendFilename, FileLoaderConfig.bRelativeToContent, Data))
		{
			return false;
		}

		Bytes.Append(MoveTemp(Data));
	}

	for (const FString& AppendString : FileLoaderConfig.AppendStrings)
	{
		TArray<uint8> Data;
		Compushady::StringToShaderCode(AppendString, Data);

		Bytes.Append(MoveTemp(Data));
	}

	return true;
}

FIntVector UCompushadyFunctionLibrary::IntToDispatchXYZ(const int32 Value, const FIntVector& ThreadGroupSize)
{
	if (ThreadGroupSize.X <= 0 || ThreadGroupSize.Y <= 0 || ThreadGroupSize.Z <= 0)
	{
		return FIntVector();
	}
	return FIntVector(FMath::DivideAndRoundUp(Value, ThreadGroupSize.X), 1, 1);
}

FIntVector UCompushadyFunctionLibrary::Int64ToDispatchXYZ(const int64 Value, const FIntVector& ThreadGroupSize)
{
	if (ThreadGroupSize.X <= 0 || ThreadGroupSize.Y <= 0 || ThreadGroupSize.Z <= 0)
	{
		return FIntVector();
	}
	return FIntVector(FMath::DivideAndRoundUp(static_cast<int32>(Value), ThreadGroupSize.X), 1, 1);
}

FIntVector UCompushadyFunctionLibrary::IntVectorToDispatchXYZ(const FIntVector& Value, const FIntVector& ThreadGroupSize)
{
	if (ThreadGroupSize.X <= 0 || ThreadGroupSize.Y <= 0 || ThreadGroupSize.Z <= 0)
	{
		return FIntVector();
	}
	return FIntVector(FMath::DivideAndRoundUp(Value.X, ThreadGroupSize.X),
		FMath::DivideAndRoundUp(Value.Y, ThreadGroupSize.Y),
		FMath::DivideAndRoundUp(Value.Z, ThreadGroupSize.Z));
}