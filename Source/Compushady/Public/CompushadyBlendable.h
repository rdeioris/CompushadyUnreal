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
	bool InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages);
	bool InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages);

	/* IBlendableInterface implementation */
	virtual void OverrideBlendableSettings(class FSceneView& View, float Weight) const override;
	/* IBlendableInterface implementation */

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool UpdateResources(const FCompushadyResourceArray& InPSResourceArray, FString& ErrorMessages);

	FPixelShaderRHIRef GetPixelShader() const;

protected:
	FPixelShaderRHIRef PixelShaderRef;

	UPROPERTY()
	FCompushadyResourceBindings PSResourceBindings;
	
	UPROPERTY()
	FCompushadyResourceArray PSResourceArray;
};