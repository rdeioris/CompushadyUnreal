// Copyright 2023 - Roberto De Ioris.


#include "CompushadyVideoEncoder.h"
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
#include "Video/VideoEncoder.h"
#include "Video/Encoders/Configs/VideoEncoderConfigH264.h"
#include "Video/Encoders/Configs/VideoEncoderConfigH265.h"
#include "Video/Resources/VideoResourceRHI.h"
struct FCompushadyVideoEncoder
{
	TSharedPtr<TVideoEncoder<FVideoResourceRHI>> Encoder;
};
#endif


bool UCompushadyVideoEncoder::Initialize(const ECompushadyVideoEncoderCodec Codec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	auto ApplyConfig = [](TSharedPtr<FVideoEncoderConfig> Config, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency)
		{
			switch (Quality)
			{
			case ECompushadyVideoEncoderQuality::Default:
				Config->Preset = EAVPreset::Default;
				break;
			case ECompushadyVideoEncoderQuality::Low:
				Config->Preset = EAVPreset::LowQuality;
				break;
			case ECompushadyVideoEncoderQuality::High:
				Config->Preset = EAVPreset::HighQuality;
				break;
			case ECompushadyVideoEncoderQuality::UltraLow:
				Config->Preset = EAVPreset::UltraLowQuality;
				break;
			case ECompushadyVideoEncoderQuality::Lossless:
				Config->Preset = EAVPreset::Lossless;
				break;
			default:
				Config->Preset = EAVPreset::Default;
				break;
			}

			switch (Latency)
			{
			case ECompushadyVideoEncoderLatency::Default:
				Config->LatencyMode = EAVLatencyMode::Default;
				break;
			case ECompushadyVideoEncoderLatency::Low:
				Config->LatencyMode = EAVLatencyMode::LowLatency;
				break;
			case ECompushadyVideoEncoderLatency::UltraLow:
				Config->LatencyMode = EAVLatencyMode::UltraLowLatency;
				break;
			default:
				Config->LatencyMode = EAVLatencyMode::Default;
				break;
			}
		};


	VideoEncoder = MakeShared<FCompushadyVideoEncoder>();
	if (!VideoEncoder)
	{
		return false;
	}

	if (Codec == ECompushadyVideoEncoderCodec::H264Main)
	{
		TSharedPtr<FVideoEncoderConfigH264> Config = MakeShared<FVideoEncoderConfigH264>();
		Config->RepeatSPSPPS = true;
		Config->Profile = EH264Profile::Main;
		ApplyConfig(Config, Quality, Latency);
		VideoEncoder->Encoder = TVideoEncoder<FVideoResourceRHI>::Create<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), *Config.Get());
	}
	else if (Codec == ECompushadyVideoEncoderCodec::H264Baseline)
	{
		TSharedPtr<FVideoEncoderConfigH264> Config = MakeShared<FVideoEncoderConfigH264>();
		Config->RepeatSPSPPS = true;
		Config->Profile = EH264Profile::Baseline;
		ApplyConfig(Config, Quality, Latency);
		VideoEncoder->Encoder = TVideoEncoder<FVideoResourceRHI>::Create<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), *Config.Get());
	}
	else if (Codec == ECompushadyVideoEncoderCodec::H264High)
	{
		TSharedPtr<FVideoEncoderConfigH264> Config = MakeShared<FVideoEncoderConfigH264>();
		Config->RepeatSPSPPS = true;
		Config->Profile = EH264Profile::High;
		ApplyConfig(Config, Quality, Latency);
		VideoEncoder->Encoder = TVideoEncoder<FVideoResourceRHI>::Create<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), *Config.Get());
	}
	else if (Codec == ECompushadyVideoEncoderCodec::H265Main)
	{
		TSharedPtr<FVideoEncoderConfigH265> Config = MakeShared<FVideoEncoderConfigH265>();
		Config->RepeatSPSPPS = true;
		Config->Profile = EH265Profile::Main;
		ApplyConfig(Config, Quality, Latency);
		VideoEncoder->Encoder = TVideoEncoder<FVideoResourceRHI>::Create<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), *Config.Get());
	}

	if (!VideoEncoder->Encoder)
	{
		VideoEncoder = nullptr;
		return false;
	}

	Timestamp = 0;

	return true;
#else
	return false;
#endif
}

bool UCompushadyVideoEncoder::EncodeFrame(UCompushadyResource* FrameResource, const bool bForceKeyFrame)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	if (!FrameResource)
	{
		return false;
	}

	if (!FrameResource->IsValidTexture())
	{
		return false;
	}

	FVideoEncoderConfigH264& Config = VideoEncoder->Encoder->GetInstance()->Edit<FVideoEncoderConfigH264>();

	Config.Width = FrameResource->GetTextureSize().X;
	Config.Height = FrameResource->GetTextureSize().Y;

	VideoEncoder->Encoder->SetMinimalConfig(Config);

	FVideoDescriptor RawDescriptor = FVideoResourceRHI::GetDescriptorFrom(VideoEncoder->Encoder->GetDevice().ToSharedRef(), FrameResource->GetTextureRHI());

	TSharedPtr<FVideoResourceRHI> VideoResource = MakeShared<FVideoResourceRHI>(
		VideoEncoder->Encoder->GetDevice().ToSharedRef(),
		FVideoResourceRHI::FRawData{ FrameResource->GetTextureRHI(), nullptr, 0 }, RawDescriptor);

	VideoEncoder->Encoder->SendFrame(VideoResource, Timestamp++, bForceKeyFrame);

	return true;
#else
	return false;
#endif
}

bool UCompushadyVideoEncoder::DequeueEncodedFrame(TArray<uint8>& FrameData)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	FVideoPacket VideoPacket;
	if (!VideoEncoder->Encoder->ReceivePacket(VideoPacket))
	{
		return false;
	}

	FrameData.Empty();
	FrameData.Append(VideoPacket.DataPtr.Get(), VideoPacket.DataSize);

	return true;
#else
	return false;
#endif
}

bool UCompushadyVideoEncoder::DequeueEncodedFrame(uint8* FrameData, int32& FrameDataSize)
{
#ifdef COMPUSHADY_SUPPORTS_VIDEO_ENCODING
	FVideoPacket VideoPacket;
	if (!VideoEncoder->Encoder->ReceivePacket(VideoPacket))
	{
		return false;
	}

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
	if (VideoEncoder)
	{
		VideoEncoder->Encoder->Close();
	}
#endif
}