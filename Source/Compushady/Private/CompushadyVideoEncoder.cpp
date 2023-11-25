// Copyright 2023 - Roberto De Ioris.


#include "CompushadyVideoEncoder.h"
#include "Video/Encoders/Configs/VideoEncoderConfigH264.h"
#include "Video/Resources/VideoResourceRHI.h"


bool UCompushadyVideoEncoder::Initialize(const ECompushadyVideoEncoderCodec Codec, const ECompushadyVideoEncoderQuality Quality, const ECompushadyVideoEncoderLatency Latency)
{

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

	TSharedPtr<FVideoEncoderConfigH264> Config = MakeShared<FVideoEncoderConfigH264>();

	if (Codec == ECompushadyVideoEncoderCodec::H264Main)
	{
		Config->RepeatSPSPPS = true;
		Config->Profile = EH264Profile::Main;
	}
	else if (Codec == ECompushadyVideoEncoderCodec::H264Baseline)
	{
		Config->RepeatSPSPPS = true;
		Config->Profile = EH264Profile::Baseline;
	}
	else if (Codec == ECompushadyVideoEncoderCodec::H264High)
	{
		Config->RepeatSPSPPS = true;
		Config->Profile = EH264Profile::High;
	}
	
	ApplyConfig(Config, Quality, Latency);

	VideoEncoder = TVideoEncoder<FVideoResourceRHI>::Create<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), *Config.Get());

	if (!VideoEncoder)
	{
		return false;
	}

	Timestamp = 0;

	return true;
}

bool UCompushadyVideoEncoder::EncodeFrame(UCompushadyResource* FrameResource, const bool bForceKeyFrame)
{
	if (!FrameResource)
	{
		return false;
	}

	if (!FrameResource->IsValidTexture())
	{
		return false;
	}

	FVideoEncoderConfigH264& Config = VideoEncoder->GetInstance()->Edit<FVideoEncoderConfigH264>();

	Config.Width = FrameResource->GetTextureSize().X;
	Config.Height = FrameResource->GetTextureSize().Y;

	VideoEncoder->SetMinimalConfig(Config);

	FVideoDescriptor RawDescriptor = FVideoResourceRHI::GetDescriptorFrom(VideoEncoder->GetDevice().ToSharedRef(), FrameResource->GetTextureRHI());

	TSharedPtr<FVideoResourceRHI> VideoResource = MakeShared<FVideoResourceRHI>(
		VideoEncoder->GetDevice().ToSharedRef(),
		FVideoResourceRHI::FRawData{ FrameResource->GetTextureRHI(), nullptr, 0 }, RawDescriptor);

	VideoEncoder->SendFrame(VideoResource, Timestamp++, bForceKeyFrame);

	return true;
}

bool UCompushadyVideoEncoder::DequeueEncodedFrame(TArray<uint8>& FrameData)
{
	FVideoPacket VideoPacket;
	if (!VideoEncoder->ReceivePacket(VideoPacket))
	{
		return false;
	}

	FrameData.Empty();
	FrameData.Append(VideoPacket.DataPtr.Get(), VideoPacket.DataSize);

	return true;
}

bool UCompushadyVideoEncoder::DequeueEncodedFrame(uint8* FrameData, int32& FrameDataSize)
{
	FVideoPacket VideoPacket;
	if (!VideoEncoder->ReceivePacket(VideoPacket))
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
}

UCompushadyVideoEncoder::~UCompushadyVideoEncoder()
{
	if (VideoEncoder)
	{
		VideoEncoder->Close();
	}
}