// Copyright 2023 - Roberto De Ioris.


#include "CompushadyUAV.h"
#include "CompushadySRV.h"

bool UCompushadyUAV::InitializeFromTexture(FTextureRHIRef InTextureRHIRef)
{
	if (!InTextureRHIRef)
	{
		return false;
	}

	TextureRHIRef = InTextureRHIRef;

	EnqueueToGPUSync(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			UAVRHIRef = COMPUSHADY_CREATE_UAV(TextureRHIRef);
		});

	if (!UAVRHIRef)
	{
		return false;
	}

	InitFence(this);

	if (InTextureRHIRef->GetOwnerName() == NAME_None)
	{
		InTextureRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::UAVCompute);

	return true;
}

bool UCompushadyUAV::InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const EPixelFormat PixelFormat)
{
	if (!InBufferRHIRef)
	{
		return false;
	}

	BufferRHIRef = InBufferRHIRef;

	EnqueueToGPUSync(
		[this, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			UAVRHIRef = COMPUSHADY_CREATE_UAV(BufferRHIRef, static_cast<uint8>(PixelFormat));
		});

	if (!UAVRHIRef)
	{
		return false;
	}

	InitFence(this);

	if (InBufferRHIRef->GetOwnerName() == NAME_None)
	{
		InBufferRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::UAVCompute);

	return true;
}

bool UCompushadyUAV::InitializeFromStructuredBuffer(FBufferRHIRef InBufferRHIRef)
{
	if (!InBufferRHIRef)
	{
		return false;
	}

	if (InBufferRHIRef->GetStride() == 0)
	{
		return false;
	}

	BufferRHIRef = InBufferRHIRef;

	EnqueueToGPUSync(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			UAVRHIRef = COMPUSHADY_CREATE_UAV(BufferRHIRef, false, false);
		});

	if (!UAVRHIRef)
	{
		return false;
	}

	InitFence(this);

	if (InBufferRHIRef->GetOwnerName() == NAME_None)
	{
		InBufferRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::UAVCompute);

	return true;
}

FUnorderedAccessViewRHIRef UCompushadyUAV::GetRHI() const
{
	return UAVRHIRef;
}