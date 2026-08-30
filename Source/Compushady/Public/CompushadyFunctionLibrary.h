// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyCBV.h"
#include "CompushadyCompute.h"
#include "CompushadyDSV.h"
#include "CompushadyShader.h"
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

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCompushadyResourceCreation, UCompushadyResource*, Resource, const FString&, ErrorMessages);

struct FCompushadyAsyncContext
{
	FCompushadyAsyncContext() = delete;
	FCompushadyAsyncContext(const FCompushadyResourceCreation& InOnResource);
	~FCompushadyAsyncContext();

	FCompushadyResourceCreation OnResource;
	UCompushadyResource* Resource = nullptr;
	FString ErrorMessages;
};

/**
 *
 */
UCLASS()
class COMPUSHADY_API UCompushadyFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Creates a Constant Buffer View (CBV) of the specified size in bytes, zero-initialized.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBV(const FString& Name, const int64 Size);

	// Creates a Constant Buffer View (CBV) initialized with the given raw byte data.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBVFromData(const FString& Name, const TArray<uint8>& Data);

	// Creates a Constant Buffer View (CBV) initialized from an array of floats.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBVFromFloatArray(const FString& Name, const TArray<float>& Data);

	// Creates a Constant Buffer View (CBV) initialized from an array of integers.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBVFromIntArray(const FString& Name, const TArray<int32>& Data);

	// Creates a read-only Shader Resource View (SRV) over a typed GPU buffer of the given size (bytes) and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBuffer(const FString& Name, const int64 Size, const EPixelFormat PixelFormat);

	// Creates a read-write Unordered Access View (UAV) over a typed GPU buffer of the given size (bytes) and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVBuffer(const FString& Name, const int64 Size, const EPixelFormat PixelFormat);

	// Creates a read-only SRV over a typed GPU buffer filled with the given float array, viewed as the specified pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const EPixelFormat PixelFormat);

	// Creates a read-only SRV over a typed GPU buffer filled with the given byte array, viewed as the specified pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromByteArray(const FString& Name, const TArray<uint8>& Data, const EPixelFormat PixelFormat);

	// Creates a read-only SRV over a typed GPU buffer filled with the raw contents of a file, viewed as the specified pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromFile(const FString& Name, const FString& Filename, const EPixelFormat PixelFormat);


	// Creates a read-only SRV over a structured buffer filled with the given float array, using the specified element stride.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const int32 Stride);

	// Creates a read-only SRV over a structured buffer filled with the given byte array (starting at Offset), using the specified element stride.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromByteArray(const FString& Name, const TArray<uint8>& Data, const int32 Stride, const int64 Offset = 0);

	// Creates a read-only SRV over a structured buffer filled with a file's contents (starting at Offset), using the specified element stride.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromFile(const FString& Name, const FString& Filename, const int32 Stride, const int64 Offset = 0);


	// Parses the given text lines into floats (selected Columns, split by Separator) and creates a structured buffer SRV from them.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromStringArray(const FString& Name, const TArray<FString>& Lines, const TArray<int32>& Columns, const FString& Separator = ",", const int32 SkipLines = 0, const bool bCullEmpty = false);

	// Loads an ASCII/CSV file, parses the selected Columns into floats, and creates a structured buffer SRV from them.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromASCIIFile(const FString& Name, const FString& Filename, const TArray<int32>& Columns, const FString& Separator = ",", const int32 SkipLines = 0, const bool bCullEmpty = false);




	// Creates a read-write UAV over a structured buffer of the given size (bytes) and element stride.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVStructuredBuffer(const FString& Name, const int64 Size, const int32 Stride);

	// Creates a read-write UAV backed by a new 2D texture of the given width, height and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format);

	// Creates a read-write UAV backed by a new 3D (volume) texture of the given width, height, depth and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format);

	// Creates a read-write UAV backed by a new 2D texture array with the given width, height, slice count and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture2DArray(const FString& Name, const int32 Width, const int32 Height, const int32 Slices, const EPixelFormat Format);

	// Creates a read-write UAV backed by a new cube texture of the given face size (Width) and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTextureCube(const FString& Name, const int32 Width, const EPixelFormat Format);

	// Creates a new UAV-enabled 2D render target and returns both it (via RenderTarget) and a read-write UAV over it.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTextureRenderTarget2D(const int32 Width, const int32 Height, const EPixelFormat Format, UTextureRenderTarget2D*& RenderTarget, const FLinearColor ClearColor = FLinearColor::Black, const float Gamma = 2.2, const bool bLinearGamma = false);

	// Creates a new 2D render target and returns both it (via RenderTarget) and a Render Target View (RTV) over it.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRTV* CreateCompushadyRTVTextureRenderTarget2D(const int32 Width, const int32 Height, const EPixelFormat Format, UTextureRenderTarget2D*& RenderTarget, const FLinearColor ClearColor = FLinearColor::Black, const float Gamma = 2.2, const bool bLinearGamma = false);

	// Creates a new UAV-enabled cube render target and returns both it (via RenderTarget) and a read-write UAV over it.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTextureRenderTargetCube(const int32 Width, const EPixelFormat Format, UTextureRenderTargetCube*& RenderTarget, const FLinearColor ClearColor = FLinearColor::Black, const float Gamma = 2.2, const bool bLinearGamma = false);

	// Creates a read-only SRV backed by a new 2D texture of the given width, height and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format);

	// Creates a read-only SRV backed by a new 3D (volume) texture of the given width, height, depth and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format);


	// Creates a read-only SRV backed by a new 2D texture filled with random bytes drawn from the given random stream.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture2DFromRandomStream(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format, const FRandomStream& RandomStream);

	// Creates a read-only SRV backed by a new 3D texture whose voxel data is loaded from a file (starting at Offset).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture3DFromFile(const FString& Name, const FString& Filename, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format, const int64 Offset = 0);


	// Creates a read-only SRV over an existing UTexture2D asset (streaming it in fully first if needed).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTexture2D(UTexture2D* Texture2D);

	// Creates a read-only SRV over an existing UTexture2DArray asset (streaming it in fully first if needed).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTexture2DArray(UTexture2DArray* Texture2DArray);

	// Creates a read-only SRV over an existing 2D render target.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	// Creates a read-only SRV over an existing media texture.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromMediaTexture(UMediaTexture* MediaTexture);

	// Creates a Render Target View (RTV) over an existing 2D render target.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRTV* CreateCompushadyRTVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	// Creates a texture sampler with the given filter, per-axis address modes and border color.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySampler* CreateCompushadySampler(const TextureFilter Filter, const ECompushadySamplerAddressMode AddressU, const ECompushadySamplerAddressMode AddressV, const ECompushadySamplerAddressMode AddressW, const FLinearColor BorderColor = FLinearColor::Black);

	// Creates an RTV backed by a new 2D texture of the given width, height, pixel format and clear color.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRTV* CreateCompushadyRTVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format, const FLinearColor ClearColor);

	// Creates a Depth Stencil View (DSV) backed by a new 2D depth texture with the given depth/stencil clear values (Format must be a depth format).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyDSV* CreateCompushadyDSVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format, const float DepthClearValue = 1.0, const int32 StencilClearValue = 0);

	// Creates a read-only SRV over an existing cube texture asset (streaming it in fully first if needed).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTextureCube(UTextureCube* TextureCube);

	// Samples a float curve at Steps evenly-spaced points between StartTime and EndTime and creates a read-only SRV buffer of the results.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromCurveFloat(const FString& Name, UCurveFloat* CurveFloat, const float StartTime, const float EndTime, const int32 Steps);

	// Creates a read-only SRV buffer from all float columns of a data table, packed row by row.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromDataTable(const FString& Name, UDataTable* DataTable, const EPixelFormat PixelFormat);

	// Creates a read-write UAV over an existing 2D render target (enabling UAV support on it if needed).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	// Creates a read-write UAV over an existing 2D render target array (enabling UAV support if needed).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray);

	// Creates a read-write UAV over an existing cube render target (enabling UAV support if needed).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTargetCube(UTextureRenderTargetCube* RenderTargetCube);

	// Creates a read-write UAV over an existing volume render target (enabling UAV support if needed).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTargetVolume(UTextureRenderTargetVolume* RenderTargetVolume);

	// Compiles a compute shader from an HLSL source file; returns any compile errors in ErrorMessages.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "FileLoaderConfig", AdvancedDisplay = "FileLoaderConfig"), Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint = "main", const FCompushadyFileLoaderConfig& FileLoaderConfig = FCompushadyFileLoaderConfig());

	// Creates a compute shader from a precompiled SPIR-V binary file; returns any errors in ErrorMessages.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromSPIRVFile(const FString& Filename, FString& ErrorMessages);

	// Creates a compute shader from a precompiled SPIR-V binary blob; returns any errors in ErrorMessages.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromSPIRVBlob(const TArray<uint8>& Blob, FString& ErrorMessages);

	// Creates a compute shader from a precompiled DXIL binary file; returns any errors in ErrorMessages.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromDXILFile(const FString& Filename, FString& ErrorMessages);

	// Creates a hardware video encoder configured with the given codec, quality and latency settings.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyVideoEncoder* CreateCompushadyVideoEncoder(const ECompushadyVideoEncoderCodec Codec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency);

	// Loads a SPIR-V binary file and disassembles it into human-readable text; returns false on failure.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleSPIRVFile(const FString& Filename, FString& Disassembled, FString& ErrorMessages);

	// Disassembles a SPIR-V binary blob into human-readable text; returns false on failure.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleSPIRVBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages);

	// Loads a DXIL binary file and disassembles it into human-readable text; returns false on failure.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleDXILFile(const FString& Filename, FString& Disassembled, FString& ErrorMessages);

	// Disassembles a DXIL binary blob into human-readable text; returns false on failure.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleDXILBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages);

	// Cross-compiles a SPIR-V binary blob back into HLSL source, also returning the detected entry point.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool SPIRVBlobToHLSL(const TArray<uint8>& Blob, FString& HLSL, FString& EntryPoint, FString& ErrorMessages);

	// Cross-compiles a SPIR-V binary blob back into GLSL source.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool SPIRVBlobToGLSL(const TArray<uint8>& Blob, FString& GLSL, FString& ErrorMessages);

	// Compiles HLSL source to a SPIR-V blob, also returning the reflected resource bindings and thread group size.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool CompileHLSLToSPIRVBlob(const FString& HLSL, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& Blob, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);

	// Compiles GLSL source to a SPIR-V blob, also returning the reflected resource bindings and thread group size.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool CompileGLSLToSPIRVBlob(const FString& GLSL, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& Blob, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);

	// Compiles a compute shader from a GLSL source file; returns any compile errors in ErrorMessages.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "FileLoaderConfig", AdvancedDisplay = "FileLoaderConfig"), Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromGLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint = "main", const FCompushadyFileLoaderConfig& FileLoaderConfig = FCompushadyFileLoaderConfig());

	// Compiles a compute shader from an in-line HLSL source string; returns any compile errors in ErrorMessages.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLString(const FString& ShaderSource, FString& ErrorMessages, const FString& EntryPoint = "main");

	// Creates a rasterizer pipeline from HLSL vertex and pixel shader sources using the given rasterizer config.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RasterizerConfig"), Category = "Compushady")
	static UCompushadyRasterizer* CreateCompushadyVSPSRasterizerFromHLSLString(const FString& VertexShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main");

	// Creates a rasterizer pipeline from HLSL mesh and pixel shader sources using the given rasterizer config.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RasterizerConfig"), Category = "Compushady")
	static UCompushadyRasterizer* CreateCompushadyMSPSRasterizerFromHLSLString(const FString& MeshShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& MeshShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main");

	// Creates a ray tracing pipeline from HLSL ray-generation, closest-hit and miss shader sources.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RasterizerConfig"), Category = "Compushady")
	static UCompushadyRayTracer* CreateCompushadyRayTracerFromHLSLString(const FString& RayGenShaderSource, const FString& RayHitShaderSource, const FString& RayMissShaderSource, FString& ErrorMessages, const FString& RayGenShaderEntryPoint = "main", const FString& RayHitShaderEntryPoint = "main", const FString& RayMissShaderEntryPoint = "main");

	// Compiles a compute shader from the HLSL code stored in a Compushady shader asset.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLShaderAsset(UCompushadyShader* ShaderAsset, FString& ErrorMessages, const FString& EntryPoint = "main");

	// Creates a read-only SRV over the selected renderer scene texture (e.g. GBuffer or depth).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromSceneTexture(const ECompushadySceneTexture SceneTexture);

	// Creates a read-only SRV over an existing Compushady resource, selecting the given slice/mip range and pixel format.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromResource(UCompushadyResource* Resource, const int32 Slice, const int32 MipLevel, const int32 NumSlices = 1, const int32 NumMips = 1, const EPixelFormat PixelFormat = EPixelFormat::PF_Unknown);

	// Creates a read-only SRV over the ray tracing acceleration structure of the current world's scene.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromWorldSceneAccelerationStructure(UObject* WorldContextObject);

	// Creates a read-only SRV over a static mesh's vertex position buffer for the given LOD; outputs the vertex count.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromStaticMeshPositionBuffer(UStaticMesh* StaticMesh, int32& NumVertices, const int32 LOD = 0);

	// Creates a read-only SRV over a static mesh's index buffer for the given LOD; outputs the index count.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromStaticMeshIndexBuffer(UStaticMesh* StaticMesh, int32& NumIndices, const int32 LOD = 0);

	// Creates a read-only SRV over a static mesh's tangent buffer for the given LOD; outputs the vertex count.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromStaticMeshTangentBuffer(UStaticMesh* StaticMesh, int32& NumVertices, const int32 LOD = 0);

	// Creates a read-only SRV over a static mesh's UV (texcoord) buffer for the given LOD; outputs the vertex count and number of UV channels.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromStaticMeshUVBuffer(UStaticMesh* StaticMesh, int32& NumVertices, int32& NumUVs, const int32 LOD = 0);

	// Compiles a compute shader from an in-line GLSL source string; returns any compile errors in ErrorMessages.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromGLSLString(const FString& ShaderSource, FString& ErrorMessages, const FString& EntryPoint = "main");

	// Creates a rasterizer pipeline from GLSL vertex and pixel shader sources using the given rasterizer config.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RasterizerConfig"), Category = "Compushady")
	static UCompushadyRasterizer* CreateCompushadyVSPSRasterizerFromGLSLString(const FString& VertexShaderSource, const FString& PixelShaderSource, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main");

	// Dispatches an ordered sequence of compute passes as a single GPU batch, firing OnSignaled once all passes complete.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Computes,OnSignaled"), Category = "Compushady")
	static void DispatchMultiPass(const TArray<FCompushadyComputePass>& ComputePasses, const FCompushadySignaled& OnSignaled);

	// Converts a double value into a Compushady float wrapper (Blueprint autocast).
	UFUNCTION(BlueprintPure, meta = (DisplayName = "To Compushady Float", BlueprintAutocast), Category = "Compushady")
	static FCompushadyFloat Conv_DoubleToCompushadyFloat(double Value);

	static void EnqueueToGPUMulti(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction, const FCompushadySignaled& OnSignaled, TArray<ICompushadyPipeline*> Pipelines);

	// Converts a linear element count into a dispatch thread-group count by dividing by the X thread group size (rounding up).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	static FIntVector IntToDispatchXYZ(const int32 Value, const FIntVector& ThreadGroupSize);

	// Converts a 64-bit linear element count into a dispatch thread-group count by dividing by the X thread group size (rounding up).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	static FIntVector Int64ToDispatchXYZ(const int64 Value, const FIntVector& ThreadGroupSize);

	// Converts a 3D element count into a dispatch thread-group count by dividing each axis by the thread group size (rounding up).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	static FIntVector IntVectorToDispatchXYZ(const FIntVector& Value, const FIntVector& ThreadGroupSize);

	static bool LoadFileWithLoaderConfig(const FString& Filename, TArray<uint8>& Bytes, const FCompushadyFileLoaderConfig& FileLoaderConfig);

	template<typename RETVALUE>
	static void CompushadyAsyncResource(const FCompushadyResourceCreation& OnResource, TFunction<TPair<bool, RETVALUE>(FString& ErrorMessages)> ThreadFunction, TFunction<UCompushadyResource* (const RETVALUE& RetValue, FString& ErrorMessages)> GameThreadFunction)
	{
		Async(EAsyncExecution::Thread, [OnResource, ThreadFunction, GameThreadFunction]()
			{
				FString ErrorMessages;
				TPair<bool, RETVALUE> AsyncRetValue = ThreadFunction(ErrorMessages);
				if (!AsyncRetValue.Key)
				{
					FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([OnResource, ErrorMessages]()
						{
							OnResource.ExecuteIfBound(nullptr, ErrorMessages);
						}, TStatId(), nullptr, ENamedThreads::GameThread);
					FTaskGraphInterface::Get().WaitUntilTaskCompletes(Task);
					return;
				}

				FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([OnResource, GameThreadFunction, &AsyncRetValue, &ErrorMessages]()
					{
						UCompushadyResource* Resource = GameThreadFunction(AsyncRetValue.Value, ErrorMessages);
						OnResource.ExecuteIfBound(Resource, ErrorMessages);
					}, TStatId(), nullptr, ENamedThreads::GameThread);
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(Task);
			});
	}
};
