// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyTypes.h"
#include "Video/VideoEncoder.h"
#include "CompushadyVideoEncoder.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyVideoEncoder : public UObject
{
	GENERATED_BODY()
	
public:

	~UCompushadyVideoEncoder();

	bool Initialize();

	UFUNCTION(BlueprintCallable, Category="Compushady")
	bool EncodeFrame(UCompushadyResource* FrameResource, const bool bForceKeyFrame);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool DequeueEncodedFrame(TArray<uint8>& FrameData);

	bool DequeueEncodedFrame(uint8* FrameData, int32& FrameDataSize);

protected:

	TSharedPtr<TVideoEncoder<class FVideoResourceRHI>> VideoEncoder;

	uint32 Timestamp;
};
