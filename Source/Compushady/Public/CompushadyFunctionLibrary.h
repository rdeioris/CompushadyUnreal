// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyCBV.h"
#include "CompushadyCompute.h"
#include "CompushadyShader.h"
#include "CompushadySoundWave.h"
#include "CompushadySRV.h"
#include "CompushadyRasterizer.h"
#include "CompushadyRTV.h"
#include "CompushadyUAV.h"
#include "Curves/CurveFloat.h"
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
	static UCompushadyUAV* CreateCompushadyUAVBuffer(const FString& Name, const int64 Size, const EPixelFormat PixelFormat);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const EPixelFormat PixelFormat);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromByteArray(const FString& Name, const TArray<uint8>& Data, const EPixelFormat PixelFormat);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromFloatArray(const FString& Name, const TArray<float>& Data, const int32 Stride);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVStructuredBuffer(const FString& Name, const int64 Size, const int32 Stride);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVTexture2DArray(const FString& Name, const int32 Width, const int32 Height, const int32 Slices, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture3D(const FString& Name, const int32 Width, const int32 Height, const int32 Depth, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVTexture2DFromImageFile(const FString& Name, const FString& Filename);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTexture2D(UTexture2D* Texture2D);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTexture2DArray(UTexture2DArray* Texture2DArray);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRTV* CreateCompushadyRTVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRTV* CreateCompushadyRTVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVFromTextureCube(UTextureCube* TextureCube);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVBufferFromCurveFloat(const FString& Name, UCurveFloat* CurveFloat, const float StartTime, const float EndTime, const int32 Steps);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTargetCube(UTextureRenderTargetCube* RenderTargetCube);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyUAV* CreateCompushadyUAVFromRenderTargetVolume(UTextureRenderTargetVolume* RenderTargetVolume);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint = "main");

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromSPIRVFile(const FString& Filename, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromDXILFile(const FString& Filename, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleSPIRVFile(const FString& Filename, FString& Disassembled, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleSPIRVBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleDXILFile(const FString& Filename, FString& Disassembled, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool DisassembleDXILBlob(const TArray<uint8>& Blob, FString& Disassembled, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static bool SPIRVBlobToHLSL(const TArray<uint8>& Blob, FString& HLSL, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromGLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint = "main");

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLString(const FString& Source, FString& ErrorMessages, const FString& EntryPoint = "main");

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyRasterizer* CreateCompushadyVSPSRasterizerFromHLSLString(const FString& VertexShaderSource, const FString& PixelShaderSource, FString& ErrorMessages, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main");

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadyCompute* CreateCompushadyComputeFromHLSLShaderAsset(UCompushadyShader* ShaderAsset, FString& ErrorMessages, const FString& EntryPoint = "main");

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySoundWave* CreateCompushadySoundWave(const UCompushadyCompute* Compute, const FCompushadyResourceArray& ResourceArray, const float Duration);

};
