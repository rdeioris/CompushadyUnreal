// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadyFunctionLibrary.h"
#include "Serialization/ArrayWriter.h"
#include "AudioDeviceManager.h"
#include "AudioMixerDevice.h"
#include "AudioBusSubsystem.h"
#include "CompushadyAudioSubsystem.h"
#include "CompushadySoundWave.h"

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

bool UCompushadyFunctionLibrary::HLSLToSPIRVBlob(const FString& HLSL, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& Blob, FString& ErrorMessages)
{
	TArray<uint8> HLSLShaderCode;
	Compushady::StringToShaderCode(HLSL, HLSLShaderCode);
	return Compushady::CompileHLSL(HLSLShaderCode, EntryPoint, TargetProfile, Blob, ErrorMessages, true);
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
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(Size, EBufferUsageFlags::ShaderResource | EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::UAVCompute, ResourceCreateInfo);
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
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(Size, EBufferUsageFlags::ShaderResource | EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::StructuredBuffer, Stride, ERHIAccess::UAVCompute, ResourceCreateInfo);
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

UCompushadyUAV* UCompushadyFunctionLibrary::CreateCompushadyUAVSharedTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format)
{
	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, Width, Height, Format);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource | ETextureCreateFlags::UAV | ETextureCreateFlags::Shared);

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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format)
{
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
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(Steps * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, sizeof(float), ERHIAccess::UAVCompute, ResourceCreateInfo);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const EPixelFormat PixelFormat)
{
	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(Data.Num() * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::SRVMask, ResourceCreateInfo);
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
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(Data.Num(), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer, GPixelFormats[PixelFormat].BlockBytes, ERHIAccess::SRVMask, ResourceCreateInfo);
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
	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, Stride](FRHICommandListImmediate& RHICmdList)
		{
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(Data.Num() * sizeof(float), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::StructuredBuffer, Stride, ERHIAccess::UAVMask, ResourceCreateInfo);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromByteArray(const FString& Name, const TArray<uint8>& Data, const int32 Stride)
{
	FBufferRHIRef BufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateBuffer)(
		[&BufferRHIRef, Name, Data, Stride](FRHICommandListImmediate& RHICmdList)
		{
			FRHIResourceCreateInfo ResourceCreateInfo(*Name);
			BufferRHIRef = COMPUSHADY_CREATE_BUFFER(Data.Num(), EBufferUsageFlags::ShaderResource | EBufferUsageFlags::StructuredBuffer, Stride, ERHIAccess::UAVMask, ResourceCreateInfo);
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromFile(const FString& Name, const FString& Filename, const int32 Stride)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}
	return CreateCompushadySRVStructuredBufferFromByteArray(Name, Data, Stride);
}

UCompushadySoundWave* UCompushadyFunctionLibrary::CreateCompushadyUAVSoundWave(const FString& Name, const float Duration, const int32 SampleRate, const int32 NumChannels, UAudioBus* AudioBus)
{
	UCompushadySoundWave* CompushadySoundWave = NewObject<UCompushadySoundWave>();

	CompushadySoundWave->Duration = Duration;
	CompushadySoundWave->SetSampleRate(SampleRate);
	CompushadySoundWave->NumChannels = NumChannels;
	CompushadySoundWave->bLooping = true;

	CompushadySoundWave->UAV = CreateCompushadyUAVBuffer(Name, SampleRate * Duration * NumChannels * sizeof(float), EPixelFormat::PF_R32_FLOAT);

	if (AudioBus)
	{
		FSoundSourceBusSendInfo BusInfo;
		BusInfo.AudioBus = AudioBus;
		CompushadySoundWave->BusSends.Add(BusInfo);
	}

	return CompushadySoundWave;
}

UCompushadySampler* UCompushadyFunctionLibrary::CreateCompushadySampler(const TextureFilter Filter, const TextureAddress AddressU, const TextureAddress AddressV, const TextureAddress AddressW)
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

	auto GetAddressMode = [](const TextureAddress Address) -> ESamplerAddressMode
		{
			switch (Address)
			{
			case(TextureAddress::TA_Clamp):
				return ESamplerAddressMode::AM_Clamp;
				break;
			case(TextureAddress::TA_Mirror):
				return ESamplerAddressMode::AM_Mirror;
				break;
			default:
				return ESamplerAddressMode::AM_Wrap;
				break;
			}
		};

	FSamplerStateInitializerRHI SamplerStateInitializer(SamplerFilter, GetAddressMode(AddressU), GetAddressMode(AddressV), GetAddressMode(AddressW));
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

UCompushadyBlendable* UCompushadyFunctionLibrary::CreateCompushadyBlendableFromHLSLString(const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation)
{
	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, ShaderCode);

	if (!CompushadyBlendable->InitFromHLSL(ShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResources(PSResourceArray, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyFunctionLibrary::CreateCompushadyBlendableFromGLSLString(const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation)
{
	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, ShaderCode);

	if (!CompushadyBlendable->InitFromGLSL(ShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResources(PSResourceArray, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVFromUAV(UCompushadyUAV* UAV)
{
	if (!UAV)
	{
		return nullptr;
	}

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();

	if (UAV->IsValidBuffer())
	{
		if (EnumHasAnyFlags(UAV->GetBufferRHI()->GetUsage(), EBufferUsageFlags::StructuredBuffer))
		{
			if (!CompushadySRV->InitializeFromStructuredBuffer(UAV->GetBufferRHI()))
			{
				return nullptr;
			}
		}
		else
		{
			if (!CompushadySRV->InitializeFromBuffer(UAV->GetBufferRHI(), UAV->GetRHI()->GetDesc().Common.Format))
			{
				return nullptr;
			}
		}
	}
	else if (UAV->IsValidTexture())
	{
		if (!CompushadySRV->InitializeFromTexture(UAV->GetTextureRHI()))
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

	ENQUEUE_RENDER_COMMAND(DoCompushadyEnqueueToGPUMulti)(
		[InFunction](FRHICommandListImmediate& RHICmdList)
		{
			InFunction(RHICmdList);
		});

	FGraphEventRef RenderThreadCompletionEvent = FFunctionGraphTask::CreateAndDispatchWhenReady([] {}, TStatId(), nullptr, ENamedThreads::GetRenderThread());
	FGraphEventArray Prerequisites = { RenderThreadCompletionEvent };
	FFunctionGraphTask::CreateAndDispatchWhenReady([OnSignaled, Pipelines]
		{
			for (ICompushadyPipeline* Pipeline : Pipelines)
			{
				Pipeline->UntrackResourcesAndUnmarkAsRunning();
			}
			OnSignaled.ExecuteIfBound(true, "");
		}, TStatId(), &Prerequisites, ENamedThreads::GameThread);
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

		if (!ComputePass.Compute->CheckResourceBindings(ComputePass.ResourceArray, OnSignaled))
		{
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

UCompushadySRV* UCompushadyFunctionLibrary::CreateCompushadySRVAudioTexture2D(UObject* WorldContextObject, const FString& Name, UAudioBus* AudioBus)
{
	if (Audio::FMixerDevice* MixerDevice = FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(WorldContextObject))
	{
		int32 NumBusChannels = AudioBus->GetNumChannels();

		int32 AudioMixerSampleRate = (int32)MixerDevice->GetSampleRate();

		// Start the audio bus (if required)
		uint32 AudioBusId = AudioBus->GetUniqueID();
		int32 NumChannels = (int32)AudioBus->AudioBusChannels + 1;

		UAudioBusSubsystem* AudioBusSubsystem = MixerDevice->GetSubsystem<UAudioBusSubsystem>();
		check(AudioBusSubsystem);
		Audio::FAudioBusKey AudioBusKey = Audio::FAudioBusKey(AudioBusId);
		AudioBusSubsystem->StartAudioBus(AudioBusKey, NumChannels, false);

		// Get an output patch for the audio bus
		int32 NumFramesPerBufferToAnalyze = MixerDevice->GetNumOutputFrames();

		UCompushadySRV* SRV = CreateCompushadySRVTexture2D(Name, NumFramesPerBufferToAnalyze, NumChannels, EPixelFormat::PF_R32_FLOAT);

		if (SRV)
		{
			Audio::FPatchOutputStrongPtr PatchOutputStrongPtr = AudioBusSubsystem->AddPatchOutputForAudioBus(AudioBusKey, NumFramesPerBufferToAnalyze, NumChannels);

			WorldContextObject->GetWorld()->GetSubsystem<UCompushadyAudioSubsystem>()->RegisterAudioTexture(SRV, PatchOutputStrongPtr);

			return SRV;
		}
	}

	return nullptr;
}