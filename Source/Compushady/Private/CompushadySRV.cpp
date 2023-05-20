// Copyright 2023 - Roberto De Ioris.


#include "CompushadySRV.h"

bool UCompushadySRV::InitializeFromTexture(FTextureRHIRef InTextureRHIRef)
{
	if (!InTextureRHIRef)
	{
		return false;
	}

	TextureRHIRef = InTextureRHIRef;

	SRVRHIRef = RHICreateShaderResourceView(TextureRHIRef, 0);
	if (!SRVRHIRef)
	{
		return false;
	}

	RHITransitionInfo = FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::SRVCompute);

	return true;
}

bool UCompushadySRV::InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const int32 Stride, const EPixelFormat PixelFormat)
{
	if (!InBufferRHIRef)
	{
		return false;
	}

	uint32 TrueStride = Stride;
	if (Stride == 0)
	{
		TrueStride = GPixelFormats[PixelFormat].BlockBytes;
	}

	BufferRHIRef = InBufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateShaderResourceView)(
		[this, TrueStride, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			if (PixelFormat == EPixelFormat::PF_Unknown)
			{
				SRVRHIRef = RHICreateShaderResourceView(BufferRHIRef);
			}
			else
			{
				SRVRHIRef = RHICreateShaderResourceView(BufferRHIRef, TrueStride, PixelFormat);
			}
		});

	FlushRenderingCommands();

	if (!SRVRHIRef)
	{
		return false;
	}

	RHITransitionInfo = FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::SRVCompute);

	return true;
}

FShaderResourceViewRHIRef UCompushadySRV::GetRHI() const
{
	return SRVRHIRef;
}

FTextureRHIRef UCompushadySRV::GetTextureRHI() const
{
	return TextureRHIRef;
}

FBufferRHIRef UCompushadySRV::GetBufferRHI() const
{
	return BufferRHIRef;
}

const FRHITransitionInfo& UCompushadySRV::GetRHITransitionInfo() const
{
	return RHITransitionInfo;
}