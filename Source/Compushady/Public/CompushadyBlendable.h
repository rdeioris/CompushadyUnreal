// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/BlendableInterface.h"
#include "CompushadyTypes.h"
#include "CompushadyShader.h"
#include "CompushadyBlendable.generated.h"

/**
 *
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyBlendable : public UObject, public ICompushadyPipeline, public IBlendableInterface
{
	GENERATED_BODY()

public:
	bool InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages);
	bool InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages);

	bool InitFromHLSLAdvanced(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages);

	/* IBlendableInterface implementation */
	virtual void OverrideBlendableSettings(class FSceneView& View, float Weight) const override;
	/* IBlendableInterface implementation */

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateResources(const FCompushadyResourceArray& InPSResourceArray, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateResourcesAdvanced(const FCompushadyResourceArray& InVSResourceArray, const FCompushadyResourceArray& InPSResourceArray, const int32 InNumVertices, const int32 InNumInstances, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateResourcesByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	FGuid AddToBlitter(UObject* WorldContextObject, const int32 Priority = 0);

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

	ECompushadyPostProcessLocation PostProcessLocation = ECompushadyPostProcessLocation::AfterTonemapping;

	int32 NumVertices = 0;
	int32 NumInstances = 0;
};
