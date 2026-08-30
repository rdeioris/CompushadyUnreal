// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/BlendableInterface.h"
#include "CompushadyTypes.h"
#include "CompushadyShader.h"
#include "CompushadyCompute.h"
#include "CompushadyBlendable.generated.h"

USTRUCT(BlueprintType)
struct COMPUSHADYBLITTER_API FCompushadyBlendableMatricesConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 ViewMatrixOffset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 ProjectionMatrixOffset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 InverseViewMatrixOffset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 InverseProjectionMatrixOffset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 ScreenSizeFloat2Offset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 ViewProjectionMatrixOffset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 InverseViewProjectionMatrixOffset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 ViewOriginFloat4Offset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 DeltaTimeFloatOffset = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 TimeFloatOffset = -1;
};

USTRUCT(BlueprintType)
struct COMPUSHADYBLITTER_API FCompushadyBlendableRasterizerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FCompushadyRasterizerConfig RasterizerConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FCompushadyBlendableMatricesConfig MatricesConfig;
};

class ICompushadyTransientBlendable
{
public:
	virtual void Disable() = 0;
};

/**
 *
 */
UCLASS(BlueprintType)
class COMPUSHADYBLITTER_API UCompushadyBlendable : public UObject, public ICompushadyPipeline, public IBlendableInterface
{
	GENERATED_BODY()

public:
	bool InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages);
	bool InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages);

	bool InitFromHLSLAdvanced(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages);
	bool InitFromHLSLCompute(const TArray<uint8>& ShaderCode, const FString& ShaderEntryPoint, const FString& TargetProfile, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages);

	/* IBlendableInterface implementation */
	virtual void OverrideBlendableSettings(class FSceneView& View, float Weight) const override;
	/* IBlendableInterface implementation */

	// Validates and rebinds the pixel-shader resource array used by this simple (pixel-only) blendable; returns false and fills ErrorMessages on mismatch.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateResources(const FCompushadyResourceArray& InPSResourceArray, FString& ErrorMessages);

	// Validates and rebinds the vertex and pixel resource arrays plus draw counts and rasterizer config for an advanced blendable; returns false on invalid counts or binding mismatch.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateResourcesAdvanced(const FCompushadyResourceArray& InVSResourceArray, const FCompushadyResourceArray& InPSResourceArray, const int32 InNumVertices, const int32 InNumInstances, const FCompushadyBlendableRasterizerConfig& InBlendableRasterizerConfig, FString& ErrorMessages);

	// Validates and rebinds the compute-shader resource array, thread-group count and matrices config for a compute blendable; returns false on invalid count or binding mismatch.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateComputeResourcesAdvanced(const FCompushadyResourceArray& InResourceArray, const FIntVector& InXYZ, const FCompushadyBlendableMatricesConfig& BlendableMatricesConfig, FString& ErrorMessages);

	// Rebinds the pixel-shader resources of a simple blendable from a name-keyed map; returns false and fills ErrorMessages on mismatch.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateResourcesByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages);

	// Rebinds the vertex and pixel resources of an advanced blendable from name-keyed maps along with draw counts and rasterizer config; returns false on mismatch.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateResourcesByMapAdvanced(const TMap<FString, TScriptInterface<ICompushadyBindable>>& InVSResourceMap, const TMap<FString, TScriptInterface<ICompushadyBindable>>& InPSResourceMap, const int32 InNumVertices, const int32 InNumInstances, const FCompushadyBlendableRasterizerConfig& InBlendableRasterizerConfig, FString& ErrorMessages);

	// Rebinds the compute-shader resources of a compute blendable from a name-keyed map along with thread-group count and matrices config; returns false on mismatch.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateComputeResourcesByMapAdvanced(const TMap<FString, TScriptInterface<ICompushadyBindable>>& InResourceMap, const FIntVector& InXYZ, const FCompushadyBlendableMatricesConfig& BlendableMatricesConfig, FString& ErrorMessages);

	// Registers this blendable as a post-process view extension with the given priority, using the supplied blitter actor or the world's blitter subsystem; returns a handle to remove it.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	FGuid AddToBlitter(UObject* WorldContextObject, const int32 Priority = 0, class ACompushadyBlitterActor* BlitterActor = nullptr);

	FPixelShaderRHIRef GetPixelShader() const;

protected:
	FPixelShaderRHIRef PixelShaderRef = nullptr;

	UPROPERTY()
	FCompushadyResourceBindings PSResourceBindings;

	UPROPERTY()
	FCompushadyResourceArray PSResourceArray;

	FVertexShaderRHIRef VertexShaderRef = nullptr;

	UPROPERTY()
	FCompushadyResourceBindings VSResourceBindings;

	UPROPERTY()
	FCompushadyResourceArray VSResourceArray;

	FComputeShaderRHIRef ComputeShaderRef = nullptr;

	UPROPERTY()
	FCompushadyResourceBindings ComputeResourceBindings;

	UPROPERTY()
	FCompushadyResourceArray ComputeResourceArray;

	FIntVector ThreadGroupSize;
	FIntVector XYZ;

	ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping;

	int32 NumVertices = 0;
	int32 NumInstances = 0;

	FCompushadyBlendableRasterizerConfig RasterizerConfig;
};
