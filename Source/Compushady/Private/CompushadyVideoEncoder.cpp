// Copyright 2023 - Roberto De Ioris.


#include "CompushadyVideoEncoder.h"
#include "Video/Encoders/Configs/VideoEncoderConfigH264.h"
#include "Video/Resources/VideoResourceRHI.h"


bool UCompushadyVideoEncoder::Initialize()
{

	FVideoEncoderConfigH264 Config;

	VideoEncoder = TVideoEncoder<FVideoResourceRHI>::Create<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), Config);

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