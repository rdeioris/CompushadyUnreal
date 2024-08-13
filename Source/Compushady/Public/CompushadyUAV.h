// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyTypes.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyUAV.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyUAV : public UCompushadyResource
{
	GENERATED_BODY()

public:
	bool InitializeFromTexture(FTextureRHIRef InTextureRHIRef);
	bool InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const EPixelFormat PixelFormat);
	bool InitializeFromStructuredBuffer(FBufferRHIRef InBufferRHIRef);

	FUnorderedAccessViewRHIRef GetRHI() const;

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void CopyToBuffer(UCompushadyUAV* DestinationBuffer, const int64 Size, const int64 DestinationOffset, const int64 SourceOffset, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool CopyToBufferSync(UCompushadyUAV* DestinationBuffer, const int64 Size, const int64 DestinationOffset, const int64 SourceOffset, FString& ErrorMessages);

protected:
	FUnorderedAccessViewRHIRef UAVRHIRef;
};
