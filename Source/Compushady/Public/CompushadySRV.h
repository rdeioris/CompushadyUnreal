// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyTypes.h"
#include "UObject/NoExportTypes.h"
#include "CompushadySRV.generated.h"

UENUM(BlueprintType)
enum class ECompushadySceneTexture : uint8
{
	None,
	SceneColorInput,
	GBufferA,
	GBufferB,
	GBufferC,
	GBufferD,
	GBufferE,
	GBufferF,
	SceneColor,
	Depth,
	CustomDepth
};

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadySRV : public UCompushadyResource
{
	GENERATED_BODY()

public:
	bool InitializeFromTexture(FTextureRHIRef InTextureRHIRef);
	bool InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const EPixelFormat PixelFormat);
	bool InitializeFromStructuredBuffer(FBufferRHIRef InBufferRHIRef);
	bool InitializeFromSceneTexture(const ECompushadySceneTexture InSceneTexture);
	bool InitializeFromWorldSceneAccelerationStructure(UWorld* World);
	
	FShaderResourceViewRHIRef GetRHI() const;
	FTextureRHIRef GetRHI(const FPostProcessMaterialInputs& PPInputs) const;

	bool IsSceneTexture() const;

protected:
	FShaderResourceViewRHIRef SRVRHIRef;

	ECompushadySceneTexture SceneTexture = ECompushadySceneTexture::None;
	
};
