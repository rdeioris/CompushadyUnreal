// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyBlendable.h"
#include "CompushadyCBV.h"
#include "CompushadyCompute.h"
#include "CompushadyDSV.h"
#include "CompushadyShader.h"
#include "CompushadySoundWave.h"
#include "CompushadySRV.h"
#include "CompushadyRasterizer.h"
#include "CompushadyRayTracer.h"
#include "CompushadyRTV.h"
#include "CompushadySampler.h"
#include "CompushadyUAV.h"
#include "CompushadyVideoEncoder.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2DArray.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTarget2DArray.h"
#include "Engine/TextureRenderTargetCube.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CompushadyFunctionLibrary.generated.h"

USTRUCT(BlueprintType)
struct FCompushadyComputePass
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	UCompushadyCompute* Compute = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FCompushadyResourceArray ResourceArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector XYZ = FIntVector::ZeroValue;
};

USTRUCT(BlueprintType)
struct FCompushadyFileLoaderConfig
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	bool bRelativeToContent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TArray<FString> PrependFiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TArray<FString> PrependStrings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TArray<FString> AppendFiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TArray<FString> AppendStrings;
};


/**
 *
 */
UCLASS()
class COMPUSHADY_API UCompushadyFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBV(const FString& Name, const int64 Size);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBVFromData(const FString& Name, const TArray<uint8>& Data);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBVFromFloatArray(const FString& Name, const TArray<float>& Data);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBVFromIntArray(const FString& Name, const TArray<int32>& Data);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVBuffer(const FString& Name, const int64 Size, const EPixelFormat PixelFormat);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const EPixelFormat PixelFormat);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromByteArray(const FString& Name, const TArray<uint8>& Data, const EPixelFormat PixelFormat);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromFile(const FString& Name, const FString& Filename, const EPixelFormat PixelFormat);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const int32 Stride);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromByteArray(const FString& Name, const TArray<uint8>& Data, const int32 Stride, const int64 Offset = 0);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromFile(const FString& Name, const FString& Filename, const int32 Stride, const int64 Offset = 0);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromASCIIFile(const FString& Name, const FString& Filename, const TArray<int32>& Columns, const FString& Separator = ",", const int32 SkipLines = 0, const bool bCullEmpty = false);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVStructuredBuffer(const FString& Name, const int64 Size, const int32 Stride);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVSharedTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture2DArray(const FString& Name, const int32 Width, const int32 Height, const int32 Slices, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTextureCube(const FString& Name, const int32 Width, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTextureRenderTarget2D(const int32 Width, const int32 Height, const EPixelFormat Format, UTextureRenderTarget2D*& RenderTarget, const FLinearColor ClearColor = FLinearColor::Black, const float Gamma = 2.2, const bool bLinearGamma = false);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRTV* CreateCompushadyRTVTextureRenderTarget2D(const int32 Width, const int32 Height, const EPixelFormat Format, UTextureRenderTarget2D*& RenderTarget, const FLinearColor ClearColor = FLinearColor::Black, const float Gamma = 2.2, const bool bLinearGamma = false);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture2DFromImageFile(const FString& Name, const FString& Filename);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture3DFromNRRDFile(const FString& Name, const FString& Filename);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTexture2D(UTexture2D* Texture2D);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTexture2DArray(UTexture2DArray* Texture2DArray);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromMediaTexture(UMediaTexture* MediaTexture);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRTV* CreateCompushadyRTVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySampler* CreateCompushadySampler(const TextureFilter Filter, const ECompushadySamplerAddressMode AddressU, const ECompushadySamplerAddressMode AddressV, const ECompushadySamplerAddressMode AddressW, const FLinearColor BorderColor = FLinearColor::Black);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRTV* CreateCompushadyRTVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format, const FLinearColor ClearColor);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyDSV* CreateCompushadyDSVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format, const float DepthClearValue = 1.0, const int32 StencilClearValue = 0);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTextureCube(UTextureCube* TextureCube);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromCurveFloat(const FString& Name, UCurveFloat* CurveFloat, const float StartTime, const float EndTime, const int32 Steps);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromDataTable(const FString& Name, UDataTable* DataTable, const EPixelFormat PixelFormat);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTargetCube(UTextureRenderTargetCube* RenderTargetCube);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTargetVolume(UTextureRenderTargetVolume* RenderTargetVolume);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "FileLoaderConfig", AdvancedDisplay = "FileLoaderConfig"), Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint = "main", const FCompushadyFileLoaderConfig& FileLoaderConfig = FCompushadyFileLoaderConfig());

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromSPIRVFile(const FString& Filename, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromSPIRVBlob(const TArray<uint8>& Blob, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromDXILFile(const FString& Filename, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyVideoEncoder* CreateCompushadyVideoEncoder(const ECompushadyVideoEncoderCodec Codec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleSPIRVFile(const FString& Filename, FString& Disassembled, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleSPIRVBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleDXILFile(const FString& Filename, FString& Disassembled, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleDXILBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool SPIRVBlobToHLSL(const TArray<uint8>& Blob, FString& HLSL, FString& EntryPoint, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool SPIRVBlobToGLSL(const TArray<uint8>& Blob, FString& GLSL, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool CompileHLSLToSPIRVBlob(const FString& HLSL, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& Blob, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool CompileGLSLToSPIRVBlob(const FString& GLSL, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& Blob, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "FileLoaderConfig", AdvancedDisplay = "FileLoaderConfig"), Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromGLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint = "main", const FCompushadyFileLoaderConfig& FileLoaderConfig = FCompushadyFileLoaderConfig());

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLString(const FString& ShaderSource, FString& ErrorMessages, const FString& EntryPoint = "main");

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RasterizerConfig"), Category = "Compushady")
	static UCompushadyRasterizer* CreateCompushadyVSPSRasterizerFromHLSLString(const FString& VertexShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main");

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RasterizerConfig"), Category = "Compushady")
	static UCompushadyRasterizer* CreateCompushadyMSPSRasterizerFromHLSLString(const FString& MeshShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& MeshShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main");

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RasterizerConfig"), Category = "Compushady")
	static UCompushadyRayTracer* CreateCompushadyRayTracerFromHLSLString(const FString& RayGenShaderSource, const FString& RayHitShaderSource, const FString& RayMissShaderSource, FString& ErrorMessages, const FString& RayGenShaderEntryPoint = "main", const FString& RayHitShaderEntryPoint = "main", const FString& RayMissShaderEntryPoint = "main");

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLShaderAsset(UCompushadyShader* ShaderAsset, FString& ErrorMessages, const FString& EntryPoint = "main");

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySoundWave* CreateCompushadyUAVSoundWave(const FString& Name, const float Duration, const int32 SampleRate = 48000, const int32 NumChannels = 2, UAudioBus* AudioBus = nullptr);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "PSResourceArray"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyBlendableFromHLSLString(const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,BlendableRasterizerConfig"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyAdvancedBlendableFromHLSLString(const FString& VertexShaderSource, const FCompushadyResourceArray& VSResourceArray, const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices = 0, const int32 NumInstances = 0, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceMap,PSResourceMap,BlendableRasterizerConfig"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyAdvancedBlendableByMapFromHLSLString(const FString& VertexShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices = 0, const int32 NumInstances = 0, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceMap,PSResourceMap,BlendableRasterizerConfig,VSFileLoaderConfig,PSFileLoaderConfig", AdvancedDisplay = "VSFileLoaderConfig,PSFileLoaderConfig"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyAdvancedBlendableByMapFromHLSLFile(const FString& VSFilename, const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const FString& PSFilename, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices = 0, const int32 NumInstances = 0, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping, const FCompushadyFileLoaderConfig& VSFileLoaderConfig = FCompushadyFileLoaderConfig(), const FCompushadyFileLoaderConfig& PSFileLoaderConfig = FCompushadyFileLoaderConfig());

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "PSResourceArray"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyBlendableFromGLSLString(const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "PSResourceArray"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyBlendableByMapFromHLSLString(const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "PSResourceArray"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyBlendableByMapFromGLSLString(const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromSceneTexture(const ECompushadySceneTexture SceneTexture);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromResource(UCompushadyResource* Resource, const int32 Slice, const int32 MipLevel, const int32 NumSlices = 1, const int32 NumMips = 1, const EPixelFormat PixelFormat = EPixelFormat::PF_Unknown);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromWorldSceneAccelerationStructure(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromStaticMeshPositionBuffer(UStaticMesh* StaticMesh, int32& NumVertices, const int32 LOD = 0);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromStaticMeshIndexBuffer(UStaticMesh* StaticMesh, int32& NumIndices, const int32 LOD = 0);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromStaticMeshTangentBuffer(UStaticMesh* StaticMesh, int32& NumVertices, const int32 LOD = 0);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromGLSLString(const FString& ShaderSource, FString& ErrorMessages, const FString& EntryPoint = "main");

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RasterizerConfig"), Category = "Compushady")
	static UCompushadyRasterizer* CreateCompushadyVSPSRasterizerFromGLSLString(const FString& VertexShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main");

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Computes,OnSignaled"), Category = "Compushady")
	static void DispatchMultiPass(const TArray<FCompushadyComputePass>& ComputePasses, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Compushady Float", BlueprintAutocast), Category = "Compushady")
	static FCompushadyFloat Conv_DoubleToCompushadyFloat(double Value);

	static void EnqueueToGPUMulti(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction, const FCompushadySignaled& OnSignaled, TArray<ICompushadyPipeline*> Pipelines);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVAudioTexture2D(UObject* WorldContextObject, const FString& Name, UAudioBus* AudioBus);


	static bool LoadFileWithLoaderConfig(const FString& Filename, TArray<uint8>& Bytes, const FCompushadyFileLoaderConfig& FileLoaderConfig);
};
