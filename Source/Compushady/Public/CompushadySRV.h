// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadySRV.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadySRV : public UObject
{
	GENERATED_BODY()

public:
	bool InitializeFromTexture(FTextureRHIRef InTextureRHIRef);
	bool InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const EPixelFormat PixelFormat);
	bool InitializeFromStructuredBuffer(FBufferRHIRef InBufferRHIRef);
	
	FShaderResourceViewRHIRef GetRHI() const;

	FTextureRHIRef GetTextureRHI() const;
	FBufferRHIRef GetBufferRHI() const;

	const FRHITransitionInfo& GetRHITransitionInfo() const;

protected:
	FTextureRHIRef TextureRHIRef;
	FBufferRHIRef BufferRHIRef;
	FShaderResourceViewRHIRef SRVRHIRef;
	FStagingBufferRHIRef StagingBufferRHIRef;
	FRHITransitionInfo RHITransitionInfo;
};
