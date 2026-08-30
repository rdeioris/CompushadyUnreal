// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PixelFormat.h"
#include "CompushadyFunctionLibrary.h" // FCompushadyResourceCreation (delegate type) + core factories
#include "CompushadyFileFormatsFunctionLibrary.generated.h"

class UCompushadySRV;
class UCompushadyResource;

/**
 * File-format factories and readbacks relocated out of the core Compushady module.
 * Depends on the core module (calls its public buffer/texture factories).
 */
UCLASS()
class COMPUSHADYFILEFORMATS_API UCompushadyFileFormatsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Loads an image file (PNG, EXR, etc.) from disk, uploads it to a 2D texture and returns an SRV bound to it.
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static UCompushadySRV* CreateCompushadySRVTexture2DFromImageFile(const FString& Name, const FString& Filename);

	// Loads an NRRD volume file from disk into a 3D texture and returns an SRV bound to it.
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static UCompushadySRV* CreateCompushadySRVTexture3DFromNRRDFile(const FString& Name, const FString& Filename);

	// Loads a gzip-compressed file, decompresses it and returns an SRV over a typed buffer using the given PixelFormat.
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static UCompushadySRV* CreateCompushadySRVBufferFromGZFile(const FString& Name, const FString& Filename, const EPixelFormat PixelFormat);

	// Loads a gzip-compressed file, decompresses it and returns an SRV over a structured buffer with the given element Stride (skipping Offset bytes).
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromGZFile(const FString& Name, const FString& Filename, const int32 Stride, const int64 Offset = 0);

	// Loads a LAS point-cloud file into a structured buffer of per-point floats and returns an SRV; bIncludeColors adds RGB alongside XYZ.
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromLASFile(const FString& Name, const FString& Filename, const bool bIncludeColors);

	// Loads a gzip-compressed ASCII/CSV file, parses the selected Columns into floats and returns an SRV over a structured buffer.
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static UCompushadySRV* CreateCompushadySRVStructuredBufferFromGZASCIIFile(const FString& Name, const FString& Filename, const TArray<int32>& Columns, const FString& Separator = ",", const int32 SkipLines = 0, const bool bCullEmpty = false);

	// Asynchronously loads and parses a gzip-compressed ASCII/CSV file into a structured-buffer SRV, delivering the result through the OnResource delegate.
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static void CreateCompushadySRVStructuredBufferFromGZASCIIFileAsync(const FString& Name, const FString& Filename, const TArray<int32>& Columns, const FCompushadyResourceCreation& OnResource, const FString& Separator = ",", const int32 SkipLines = 0, const bool bCullEmpty = false);

	// Reads back an RGBA8/BGRA8 texture resource and saves it to disk as a PNG file, returning true on success (ErrorMessages describes failures).
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static bool ReadbackTextureToPNGFile(UCompushadyResource* Resource, const FString& Filename, FString& ErrorMessages);

	// Reads back a texture resource and saves it to disk as a TIFF file with the given ImageDescription, returning true on success.
	UFUNCTION(BlueprintCallable, Category = "Compushady|FileFormats")
	static bool ReadbackTextureToTIFFFile(UCompushadyResource* Resource, const FString& Filename, const FString& ImageDescription, FString& ErrorMessages);

	// Asynchronously reads back an RGBA8/BGRA8 texture resource and saves it as a PNG file, invoking OnSignaled when finished.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady|FileFormats")
	static void ReadbackTextureToPNGFileAsync(UCompushadyResource* Resource, const FString& Filename, const FCompushadySignaled& OnSignaled);

	// Asynchronously reads back a texture resource and saves it as a TIFF file with the given ImageDescription, invoking OnSignaled when finished.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady|FileFormats")
	static void ReadbackTextureToTIFFFileAsync(UCompushadyResource* Resource, const FString& Filename, const FString& ImageDescription, const FCompushadySignaled& OnSignaled);
};
