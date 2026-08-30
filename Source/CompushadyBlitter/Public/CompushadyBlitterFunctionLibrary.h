// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CompushadyTypes.h"
#include "CompushadyFunctionLibrary.h"
#include "CompushadyBlendable.h"
#include "CompushadyBlitterFunctionLibrary.generated.h"

class UCompushadyResource;
class UCompushadyCBV;

/**
 * Blitter + Blendable Blueprint nodes relocated out of the core Compushady module.
 * The Draw* and viewport-matrix nodes previously lived on UCompushadyResource / UCompushadyCBV;
 * the blendable factories previously lived on UCompushadyFunctionLibrary.
 */
UCLASS()
class COMPUSHADYBLITTER_API UCompushadyBlitterFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Blits a Compushady resource onto the viewport as a quad after the whole scene has been rendered; returns a handle that can be passed to RemoveDrawable.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady|Blitter")
	static FGuid Draw(UCompushadyResource* Resource, UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	// Blits a Compushady resource onto the scene color as a quad before post-processing runs; returns a handle that can be passed to RemoveDrawable.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady|Blitter")
	static FGuid DrawBeforePostProcessing(UCompushadyResource* Resource, UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	// Blits a Compushady resource onto the scene as a quad right after the motion blur pass; returns a handle that can be passed to RemoveDrawable.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady|Blitter")
	static FGuid DrawAfterMotionBlur(UCompushadyResource* Resource, UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	// Writes the viewport's current projection matrix into the CBV at the given byte offset, optionally transposed and/or inverted; returns false on invalid input.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady|Blitter")
	static bool SetProjectionMatrixFromViewport(UCompushadyCBV* CBV, UObject* WorldContextObject, const int64 Offset, const bool bTranspose = false, const bool bInverse = false);

	// Writes the viewport's current view matrix into the CBV at the given byte offset, optionally transposed and/or inverted; returns false on invalid input.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady|Blitter")
	static bool SetViewMatrixFromViewport(UCompushadyCBV* CBV, UObject* WorldContextObject, const int64 Offset, const bool bTranspose = false, const bool bInverse = false);

	// Compiles an HLSL pixel shader into a post-process Blendable bound to the given resource array; returns null and fills ErrorMessages on failure.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "PSResourceArray"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyBlendableFromHLSLString(const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	/**
	 * Compiles HLSL vertex and pixel shaders into a rasterizing post-process Blendable that draws NumVertices x NumInstances primitives.
	 * Returns null and fills ErrorMessages on failure.
	 */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,BlendableRasterizerConfig"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyAdvancedBlendableFromHLSLString(const FString& VertexShaderSource, const FCompushadyResourceArray& VSResourceArray, const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices = 0, const int32 NumInstances = 0, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	/**
	 * Compiles HLSL vertex and pixel shaders into a rasterizing post-process Blendable, binding resources by name via the supplied maps.
	 * Returns null and fills ErrorMessages on failure.
	 */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceMap,PSResourceMap,BlendableRasterizerConfig"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyAdvancedBlendableByMapFromHLSLString(const FString& VertexShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices = 0, const int32 NumInstances = 0, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	/**
	 * Compiles an HLSL compute shader into a post-process Blendable dispatched with the given XYZ thread-group count, binding resources by name.
	 * Returns null and fills ErrorMessages on failure.
	 */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceMap,BlendableMatricesConfig"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyComputeBlendableByMapFromHLSLString(const FString& ShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, FString& ErrorMessages, const FCompushadyBlendableMatricesConfig& BlendableMatricesConfig, const FIntVector& XYZ, const FString& ShaderEntryPoint = "main", const FString& TargetProfile = "cs_6_0", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	/**
	 * Loads an HLSL compute shader from a file and builds a post-process Blendable dispatched with the given XYZ thread-group count, binding resources by name.
	 * Returns null and fills ErrorMessages on failure.
	 */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceMap,BlendableMatricesConfig,FileLoaderConfig"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyComputeBlendableByMapFromHLSLFile(const FString& Filename, const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, FString& ErrorMessages, const FCompushadyBlendableMatricesConfig& BlendableMatricesConfig, const FIntVector& XYZ, const FString& ShaderEntryPoint = "main", const FString& TargetProfile = "cs_6_0", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping, const FCompushadyFileLoaderConfig& FileLoaderConfig = FCompushadyFileLoaderConfig());

	/**
	 * Loads HLSL vertex and pixel shaders from separate files and builds a rasterizing post-process Blendable, binding resources by name.
	 * Returns null and fills ErrorMessages on failure.
	 */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceMap,PSResourceMap,BlendableRasterizerConfig,VSFileLoaderConfig,PSFileLoaderConfig", AdvancedDisplay = "VSFileLoaderConfig,PSFileLoaderConfig"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyAdvancedBlendableByMapFromHLSLFile(const FString& VSFilename, const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const FString& PSFilename, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices = 0, const int32 NumInstances = 0, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping, const FCompushadyFileLoaderConfig& VSFileLoaderConfig = FCompushadyFileLoaderConfig(), const FCompushadyFileLoaderConfig& PSFileLoaderConfig = FCompushadyFileLoaderConfig());

	// Compiles a GLSL pixel shader into a post-process Blendable bound to the given resource array; returns null and fills ErrorMessages on failure.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "PSResourceArray"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyBlendableFromGLSLString(const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	// Compiles an HLSL pixel shader into a post-process Blendable, binding resources by name via the supplied map; returns null and fills ErrorMessages on failure.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "PSResourceMap"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyBlendableByMapFromHLSLString(const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);

	// Compiles a GLSL pixel shader into a post-process Blendable, binding resources by name via the supplied map; returns null and fills ErrorMessages on failure.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "PSResourceMap"), Category = "Compushady")
	static UCompushadyBlendable* CreateCompushadyBlendableByMapFromGLSLString(const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FString& PixelShaderEntryPoint = "main", const ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping);
};
