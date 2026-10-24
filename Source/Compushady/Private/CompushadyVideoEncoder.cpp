// Copyright 2023 - Roberto De Ioris.


#include "CompushadyVideoEncoder.h"
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
#include "Containers/Queue.h"
#include "Video/VideoEncoder.h"
#include "Video/Encoders/Configs/VideoEncoderConfigAV1.h"
#include "Video/Encoders/Configs/VideoEncoderConfigH264.h"
#include "Video/Encoders/Configs/VideoEncoderConfigH265.h"
#include "Video/Resources/VideoResourceRHI.h"
// The textures frames are copied into for the encoder, used in turn. Render thread only.
struct FCompushadyEncoderTextures
{
	static constexpr int32 Count = 4;

	TArray<TSharedPtr<FVideoResourceRHI>> Textures;
	// Only the format and size: FVideoDescriptor's copy assignment shares its RawDescriptor (see Next).
	EVideoFormat Format = EVideoFormat::BGRA;
	uint32 Width = 0;
	uint32 Height = 0;
	int32 NextIndex = 0;

	// The next texture for frames of InDescriptor, (re)made when the size or format changes; null on failure.
	TSharedPtr<FVideoResourceRHI> Next(const TSharedPtr<FAVDevice>& Device, const FVideoDescriptor& InDescriptor)
	{
		check(IsInRenderingThread());
		if (Textures.Num() == 0 || Format != InDescriptor.Format || Width != InDescriptor.Width || Height != InDescriptor.Height)
		{
			Textures.Empty();
			for (int32 Index = 0; Index < Count; Index++)
			{
				// Create writes a RawDescriptor it allocates into the descriptor it is given, so give it its own.
				// Not sRGB: D3D12 makes sRGB textures typeless, and NVENC takes only B8G8R8A8_UNORM. The copied
				// bytes are already gamma encoded, which is what the encoder wants.
				FVideoDescriptor Wanted(InDescriptor.Format, InDescriptor.Width, InDescriptor.Height);
				TSharedPtr<FVideoResourceRHI> Texture = FVideoResourceRHI::Create(Device, Wanted, ETextureCreateFlags::None, false);
				if (!Texture)
				{
					Textures.Empty();
					return nullptr;
				}
				Textures.Add(Texture);
			}
			Format = InDescriptor.Format;
			Width = InDescriptor.Width;
			Height = InDescriptor.Height;
			NextIndex = 0;
		}
		TSharedPtr<FVideoResourceRHI> Texture = Textures[NextIndex];
		NextIndex = (NextIndex + 1) % Textures.Num();
		return Texture;
	}
};

// A frame copied into an encoder texture on the render thread, waiting for the GPU to finish the copy.
struct FCompushadyCopiedFrame
{
	TSharedPtr<FVideoResourceRHI> Texture;
	FGPUFenceRHIRef Fence;
	uint32 Index;
	bool bForceKeyFrame;
};

struct FCompushadyVideoEncoder
{
	TSharedPtr<TVideoEncoder<FVideoResourceRHI>> Encoder;
	TSharedPtr<FCompushadyEncoderTextures, ESPMode::ThreadSafe> Textures;
	// Filled by the render thread, sent to the encoder by the game thread once each copy is done.
	TSharedPtr<TQueue<FCompushadyCopiedFrame, EQueueMode::Spsc>, ESPMode::ThreadSafe> Copied;
	// Frames enqueued and not sent yet: never more than there are encoder textures.
	int32 InFlight = 0;

	// Sends the frames whose copy the GPU has finished, oldest first (the encoder reads them at once).
	void SendCopied()
	{
		FCompushadyCopiedFrame* Frame;
		while (Copied && (Frame = Copied->Peek()) != nullptr && (!Frame->Fence || Frame->Fence->Poll()))
		{
			if (Frame->Texture && Encoder->SendFrame(Frame->Texture, Frame->Index, Frame->bForceKeyFrame).IsNotSuccess())
			{
				UE_LOG(LogCompushady, Warning, TEXT("Video encoder: frame %u was not accepted"), Frame->Index);
			}
			Copied->Pop();
			InFlight--;
		}
	}
};

namespace
{
	// How many frames may wait for their packet before the oldest bookkeeping is dropped.
	constexpr int32 MaxPendingFrames = 256;

	void ApplyQualityAndLatency(FVideoEncoderConfig& Config, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency)
	{
		switch (Quality)
		{
		case ECompushadyVideoEncoderQuality::Default:
			Config.Preset = EAVPreset::Default;
			break;
		case ECompushadyVideoEncoderQuality::Low:
			Config.Preset = EAVPreset::LowQuality;
			break;
		case ECompushadyVideoEncoderQuality::High:
			Config.Preset = EAVPreset::HighQuality;
			break;
		case ECompushadyVideoEncoderQuality::UltraLow:
			Config.Preset = EAVPreset::UltraLowQuality;
			break;
		case ECompushadyVideoEncoderQuality::Lossless:
			Config.Preset = EAVPreset::Lossless;
			break;
		default:
			Config.Preset = EAVPreset::Default;
			break;
		}

		switch (Latency)
		{
		case ECompushadyVideoEncoderLatency::Default:
			Config.LatencyMode = EAVLatencyMode::Default;
			break;
		case ECompushadyVideoEncoderLatency::Low:
			Config.LatencyMode = EAVLatencyMode::LowLatency;
			break;
		case ECompushadyVideoEncoderLatency::UltraLow:
			Config.LatencyMode = EAVLatencyMode::UltraLowLatency;
			break;
		default:
			Config.LatencyMode = EAVLatencyMode::Default;
			break;
		}
	}

	// Builds the typed config of Codec and hands it to Visitor; returns Fallback for an unknown codec.
	// Every codec repeats its stream headers before each key frame, so a receiver can join mid-stream.
	template<typename TResult, typename TVisitor>
	TResult WithConfig(const ECompushadyVideoEncoderCodec Codec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency, TResult Fallback, TVisitor&& Visitor)
	{
		auto H264 = [&](const EH264Profile Profile)
			{
				FVideoEncoderConfigH264 Config;
				Config.RepeatSPSPPS = true;
				Config.Profile = Profile;
				// Left on Auto, NVENC turns on what the profile does not allow and writes a High stream:
				// Baseline has neither CABAC nor the 8x8 transform, Main has no 8x8 transform.
				if (Profile != EH264Profile::High)
				{
					Config.AdaptiveTransformMode = EH264AdaptiveTransformMode::Disable;
				}
				if (Profile == EH264Profile::Baseline)
				{
					Config.EntropyCodingMode = EH264EntropyCodingMode::CAVLC;
				}
				ApplyQualityAndLatency(Config, Quality, Latency);
				return Visitor(Config);
			};

		switch (Codec)
		{
		case ECompushadyVideoEncoderCodec::H264Main:
			return H264(EH264Profile::Main);
		case ECompushadyVideoEncoderCodec::H264Baseline:
			return H264(EH264Profile::Baseline);
		case ECompushadyVideoEncoderCodec::H264High:
			return H264(EH264Profile::High);
		case ECompushadyVideoEncoderCodec::H265Main:
		{
			FVideoEncoderConfigH265 Config;
			Config.RepeatSPSPPS = true;
			Config.Profile = EH265Profile::Main;
			ApplyQualityAndLatency(Config, Quality, Latency);
			return Visitor(Config);
		}
		case ECompushadyVideoEncoderCodec::AV1Main:
		{
			FVideoEncoderConfigAV1 Config;
			Config.RepeatSeqHdr = true;
			Config.Profile = EAV1Profile::Main;
			ApplyQualityAndLatency(Config, Quality, Latency);
			return Visitor(Config);
		}
		default:
			return Fallback;
		}
	}

	// Microseconds since the Unix epoch, so packets from different machines line up.
	int64 NowMicroseconds()
	{
		return (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTicks() / ETimespan::TicksPerMicrosecond;
	}
}
#endif


bool UCompushadyVideoEncoder::Initialize(const ECompushadyVideoEncoderCodec InCodec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	// The encoder itself is made at the first frame, when its size is known (see EncodeFrameAtTime).
	if (!IsCodecSupported(InCodec))
	{
		VideoEncoder = nullptr;
		return false;
	}

	VideoEncoder = MakeShared<FCompushadyVideoEncoder>();
	Codec = InCodec;
	EncoderQuality = Quality;
	EncoderLatency = Latency;
	Timestamp = 0;
	PendingFrames.Empty();
	ConfiguredSize = FIntPoint::ZeroValue;

	return true;
#else
	return false;
#endif
}

bool UCompushadyVideoEncoder::IsCodecSupported(const ECompushadyVideoEncoderCodec InCodec)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	// A registered encoder does not mean this GPU runs it (NVENC registers AV1 on GPUs without AV1 encoding):
	// open a small one and close it again.
	return WithConfig<bool>(InCodec, ECompushadyVideoEncoderQuality::Default, ECompushadyVideoEncoderLatency::Default, false, [](auto& Config)
		{
			Config.Width = 256;
			Config.Height = 256;
			TSharedPtr<TVideoEncoder<FVideoResourceRHI>> Encoder = TVideoEncoder<FVideoResourceRHI>::Create<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), Config);
			if (!Encoder)
			{
				return false;
			}
			Encoder->Close();
			return true;
		});
#else
	return false;
#endif
}

bool UCompushadyVideoEncoder::EncodeFrame(UCompushadyResource* FrameResource, const bool bForceKeyFrame)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	return EncodeFrameAtTime(FrameResource, bForceKeyFrame, NowMicroseconds());
#else
	return false;
#endif
}

bool UCompushadyVideoEncoder::EncodeFrameAtTime(UCompushadyResource* FrameResource, const bool bForceKeyFrame, const int64 CaptureTimeMicroseconds)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	if (!VideoEncoder)
	{
		return false;
	}

	if (!FrameResource)
	{
		return false;
	}

	if (!FrameResource->IsValidTexture())
	{
		return false;
	}

	// The hardware encoders take these and nothing else (NVENC: ARGB, ARGB10, NV12, YUV420 10 bit).
	const EPixelFormat Format = FrameResource->GetTextureRHI()->GetDesc().Format;
	if (Format != PF_B8G8R8A8 && Format != PF_A2B10G10R10 && Format != PF_NV12 && Format != PF_P010)
	{
		UE_LOG(LogCompushady, Warning, TEXT("Video encoder: texture format %s cannot be encoded, use B8G8R8A8 (or A2B10G10R10, NV12, P010)"), GetPixelFormatString(Format));
		return false;
	}

	// The encoder is made for the frame size with its full typed config: setting the size later through the
	// codec-neutral config resets the profile to Auto (NVENC then picks High for a Baseline encoder).
	const FIntPoint Size(FrameResource->GetTextureSize().X, FrameResource->GetTextureSize().Y);
	VideoEncoder->SendCopied();
	if (!VideoEncoder->Encoder || Size != ConfiguredSize)
	{
		if (VideoEncoder->InFlight > 0)
		{
			// Frames of the old size still wait for the old encoder.
			return false;
		}
		if (VideoEncoder->Encoder)
		{
			VideoEncoder->Encoder->Close();
		}
		VideoEncoder->Encoder = WithConfig<TSharedPtr<TVideoEncoder<FVideoResourceRHI>>>(Codec, EncoderQuality, EncoderLatency, nullptr, [&Size](auto& Config)
			{
				Config.Width = Size.X;
				Config.Height = Size.Y;
				return TVideoEncoder<FVideoResourceRHI>::Create<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), Config);
			});
		ConfiguredSize = Size;
		if (!VideoEncoder->Encoder)
		{
			ConfiguredSize = FIntPoint::ZeroValue;
			UE_LOG(LogCompushady, Warning, TEXT("Video encoder: could not create a %dx%d encoder"), Size.X, Size.Y);
			return false;
		}
	}

	// The encoder needs textures it can share with the codec (D3D12: shared committed resources; Vulkan:
	// external memory), which an ordinary render target is not. So every frame is copied into one of a few
	// textures made by AVCodecs for that, used in turn.
	// They are made and used on the render thread only: made from the game thread, AVCodecs waits for a GPU
	// fence that the render thread submits only at the end of the frame, which never comes.
	// The encoder reads a texture as soon as it gets it, so it gets one only once the GPU has finished the copy
	// (like Pixel Streaming, which encodes captured frames only): the render thread copies and writes a fence,
	// and the game thread sends the frames whose fence has passed, here and when packets are dequeued.
	const FVideoDescriptor Descriptor = FVideoResourceRHI::GetDescriptorFrom(VideoEncoder->Encoder->GetDevice().ToSharedRef(), FrameResource->GetTextureRHI());
	if (!VideoEncoder->Textures)
	{
		VideoEncoder->Textures = MakeShared<FCompushadyEncoderTextures>();
		VideoEncoder->Copied = MakeShared<TQueue<FCompushadyCopiedFrame, EQueueMode::Spsc>>();
	}

	VideoEncoder->SendCopied();
	if (VideoEncoder->InFlight >= FCompushadyEncoderTextures::Count)
	{
		// Every encoder texture still waits for its copy: this frame has nowhere to go.
		return false;
	}

	const uint32 Index = Timestamp++;
	PendingFrames.Add(Index, { CaptureTimeMicroseconds, Size });
	if (PendingFrames.Num() > MaxPendingFrames)
	{
		// The encoder dropped frames: forget the oldest ones so the map cannot grow forever.
		PendingFrames.Remove(Index - MaxPendingFrames);
	}

	VideoEncoder->InFlight++;
	ENQUEUE_RENDER_COMMAND(CompushadyEncodeFrame)(
		[Device = VideoEncoder->Encoder->GetDevice(), Textures = VideoEncoder->Textures, Copied = VideoEncoder->Copied, Source = FrameResource->GetTextureRHI(), Descriptor, Index, bForceKeyFrame](FRHICommandListImmediate& RHICmdList)
		{
			TSharedPtr<FVideoResourceRHI> EncoderTexture = Textures->Next(Device, Descriptor);
			if (!EncoderTexture)
			{
				UE_LOG(LogCompushady, Warning, TEXT("Video encoder: could not create an encoder texture of %ux%u"), Descriptor.Width, Descriptor.Height);
				// Still queued, without a texture, so the game thread's count stays right.
				Copied->Enqueue({ nullptr, nullptr, Index, bForceKeyFrame });
				return;
			}
			FTextureRHIRef Destination = EncoderTexture->GetRaw().Texture;
			RHICmdList.Transition(FRHITransitionInfo(Source, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(Destination, ERHIAccess::Unknown, ERHIAccess::CopyDest));
			RHICmdList.CopyTexture(Source, Destination, FRHICopyTextureInfo());
			RHICmdList.Transition(FRHITransitionInfo(Source, ERHIAccess::CopySrc, ERHIAccess::SRVMask));
			RHICmdList.Transition(FRHITransitionInfo(Destination, ERHIAccess::CopyDest, ERHIAccess::Present));
			FGPUFenceRHIRef Fence = RHICreateGPUFence(TEXT("CompushadyVideoEncoderCopy"));
			RHICmdList.WriteGPUFence(Fence);
			Copied->Enqueue({ EncoderTexture, Fence, Index, bForceKeyFrame });
		});

	return true;
#else
	return false;
#endif
}

bool UCompushadyVideoEncoder::DequeueEncodedPacket(FCompushadyVideoPacket& Packet)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	if (!VideoEncoder || !VideoEncoder->Encoder)
	{
		return false;
	}

	VideoEncoder->SendCopied();

	FVideoPacket VideoPacket;
	if (!VideoEncoder->Encoder->ReceivePacket(VideoPacket))
	{
		return false;
	}

	Packet.Data.Empty(VideoPacket.DataSize);
	Packet.Data.Append(VideoPacket.DataPtr.Get(), VideoPacket.DataSize);
	Packet.bKeyFrame = VideoPacket.bIsKeyframe;
	Packet.Codec = Codec;

	FPendingFrame Frame;
	if (PendingFrames.RemoveAndCopyValue(static_cast<uint32>(VideoPacket.Timestamp), Frame))
	{
		Packet.CaptureTimeMicroseconds = Frame.CaptureTimeMicroseconds;
		Packet.Width = Frame.Size.X;
		Packet.Height = Frame.Size.Y;
	}
	else
	{
		Packet.CaptureTimeMicroseconds = NowMicroseconds();
		Packet.Width = ConfiguredSize.X;
		Packet.Height = ConfiguredSize.Y;
	}

	return true;
#else
	return false;
#endif
}

bool UCompushadyVideoEncoder::DequeueEncodedFrame(TArray<uint8>& FrameData)
{
	FCompushadyVideoPacket Packet;
	if (!DequeueEncodedPacket(Packet))
	{
		return false;
	}

	FrameData = MoveTemp(Packet.Data);

	return true;
}

bool UCompushadyVideoEncoder::DequeueEncodedFrame(uint8* FrameData, int32& FrameDataSize)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	if (!VideoEncoder || !VideoEncoder->Encoder)
	{
		return false;
	}

	VideoEncoder->SendCopied();

	FVideoPacket VideoPacket;
	if (!VideoEncoder->Encoder->ReceivePacket(VideoPacket))
	{
		return false;
	}

	PendingFrames.Remove(static_cast<uint32>(VideoPacket.Timestamp));

	if (VideoPacket.DataSize > FrameDataSize)
	{
		return false;
	}

	FMemory::Memcpy(FrameData, VideoPacket.DataPtr.Get(), VideoPacket.DataSize);
	FrameDataSize = VideoPacket.DataSize;

	return true;
#else
	return false;
#endif
}

UCompushadyVideoEncoder::~UCompushadyVideoEncoder()
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	if (VideoEncoder && VideoEncoder->Encoder)
	{
		VideoEncoder->Encoder->Close();
	}
#endif
}
