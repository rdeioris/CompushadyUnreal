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
	virtual void OverrideBlendableSettings(class FSceneView& View, float Weight) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Compushady")
	UCompushadyShader* ShaderAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TEnumAsByte<EBlendableLocation> BlendableLocation;

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
