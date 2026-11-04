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
	AV1Main UMETA(DisplayName = "AV1 / Main"),
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
 * One encoded access unit: H.264/H.265 in Annex B (start codes, SPS/PPS before every key frame),
 * AV1 as low-overhead OBUs.
 */
USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyVideoPacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Compushady")
	TArray<uint8> Data;

	UPROPERTY(BlueprintReadOnly, Category = "Compushady")
	bool bKeyFrame = false;

	// The capture time given to EncodeFrameAtTime (or the time EncodeFrame was called), in microseconds.
	UPROPERTY(BlueprintReadOnly, Category = "Compushady")
	int64 CaptureTimeMicroseconds = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Compushady")
	int32 Width = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Compushady")
	int32 Height = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Compushady")
	ECompushadyVideoEncoderCodec Codec = ECompushadyVideoEncoderCodec::H264Main;
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

	bool Initialize(const ECompushadyVideoEncoderCodec InCodec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency);

	// Whether the hardware encoder of this machine can encode Codec from GPU textures (NVENC, AMF; on Linux only NVENC).
	static bool IsCodecSupported(const ECompushadyVideoEncoderCodec InCodec);

	// Submits the texture in FrameResource to the encoder as the next video frame, optionally forcing it to be encoded as a key frame; returns false if the resource is not a valid texture.
	UFUNCTION(BlueprintCallable, Category="Compushady")
	bool EncodeFrame(UCompushadyResource* FrameResource, const bool bForceKeyFrame);

	// Like EncodeFrame, but the packet carries CaptureTimeMicroseconds (for example the simulation time of the frame) instead of the time of the call.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool EncodeFrameAtTime(UCompushadyResource* FrameResource, const bool bForceKeyFrame, const int64 CaptureTimeMicroseconds);

	// Retrieves the next available encoded video packet into FrameData; returns false if no encoded frame is ready.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool DequeueEncodedFrame(TArray<uint8>& FrameData);

	bool DequeueEncodedFrame(uint8* FrameData, int32& FrameDataSize);

	// Retrieves the next available encoded video packet with its key frame flag, capture time and size; returns false if no encoded frame is ready.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool DequeueEncodedPacket(FCompushadyVideoPacket& Packet);

	UFUNCTION(BlueprintPure, Category = "Compushady")
	ECompushadyVideoEncoderCodec GetCodec() const { return Codec; }

protected:

	TSharedPtr<FCompushadyVideoEncoder> VideoEncoder;

	ECompushadyVideoEncoderCodec Codec = ECompushadyVideoEncoderCodec::H264Main;
	ECompushadyVideoEncoderQuality EncoderQuality = ECompushadyVideoEncoderQuality::Default;
	ECompushadyVideoEncoderLatency EncoderLatency = ECompushadyVideoEncoderLatency::Default;

	// The index of the next frame, passed to the encoder as its timestamp.
	uint32 Timestamp;

	struct FPendingFrame
	{
		int64 CaptureTimeMicroseconds;
		FIntPoint Size;
	};

	// The frames sent but not dequeued yet, by index.
	TMap<uint32, FPendingFrame> PendingFrames;

	FIntPoint ConfiguredSize = FIntPoint::ZeroValue;
};
