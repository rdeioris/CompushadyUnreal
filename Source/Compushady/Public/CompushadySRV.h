// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyTypes.h"
#include "UObject/NoExportTypes.h"
#include "CompushadySRV.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadySRV : public UCompushadyResource
{
	GENERATED_BODY()

public:
	bool InitializeFromTexture(FTextureRHIRef InTextureRHIRef);
	bool InitializeFromTextureAdvanced(FTextureRHIRef InTextureRHIRef, const int32 Slice, const int32 SlicesNum, const int32 MipLevel, const int32 MipsNum, const EPixelFormat PixelFormat);
	bool InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const EPixelFormat PixelFormat);
	bool InitializeFromStructuredBuffer(FBufferRHIRef InBufferRHIRef);
	bool InitializeFromSceneTexture(const ECompushadySceneTexture InSceneTexture);
	bool InitializeFromWorldSceneAccelerationStructure(UWorld* World);
	
	FShaderResourceViewRHIRef GetRHI() const;
	TPair<FShaderResourceViewRHIRef, FTextureRHIRef> GetRHI(const FCompushadySceneTextures& SceneTextures) const;

	bool IsSceneTexture() const;

protected:
	FShaderResourceViewRHIRef SRVRHIRef;

	ECompushadySceneTexture SceneTexture = ECompushadySceneTexture::None;
	
};
