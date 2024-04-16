// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyTypes.h"
#include "CompushadyVideoEncoder.generated.h"

struct FCompushadyVideoEncoder;

UENUM(BlueprintType)
enum class ECompushadyVideoEncoderCodec : uint8
{
	H264Main UMETA(DisplayName = "H.264 / Main"),
	H264Baseline UMETA(DisplayName = "H.264 / Baseline"),
	H264High UMETA(DisplayName = "H.264 / High"),
	H265Main UMETA(DisplayName = "H.265 / Main"),
};

UENUM(BlueprintType)
enum class ECompushadyVideoEncoderQuality : uint8
{
	Default,
	High,
	Low,
	UltraLow,
	Lossless
};

UENUM(BlueprintType)
enum class ECompushadyVideoEncoderLatency : uint8
{
	Default,
	Low,
	UltraLow
};

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyVideoEncoder : public UObject
{
	GENERATED_BODY()
	
public:

	~UCompushadyVideoEncoder();

	bool Initialize(const ECompushadyVideoEncoderCodec Codec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency);

	UFUNCTION(BlueprintCallable, Category="Compushady")
	bool EncodeFrame(UCompushadyResource* FrameResource, const bool bForceKeyFrame);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool DequeueEncodedFrame(TArray<uint8>& FrameData);

	bool DequeueEncodedFrame(uint8* FrameData, int32& FrameDataSize);

protected:

	TSharedPtr<FCompushadyVideoEncoder> VideoEncoder;

	uint32 Timestamp;
};
