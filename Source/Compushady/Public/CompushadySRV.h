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
class COMPUSHADY_API UCompushadySRV : public UObject, public ICompushadyResource
{
	GENERATED_BODY()

public:
	bool InitializeFromTexture(FTextureRHIRef InTextureRHIRef);
	bool InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const EPixelFormat PixelFormat);
	bool InitializeFromStructuredBuffer(FBufferRHIRef InBufferRHIRef);
	
	FShaderResourceViewRHIRef GetRHI() const;

protected:
	FShaderResourceViewRHIRef SRVRHIRef;
	
};
