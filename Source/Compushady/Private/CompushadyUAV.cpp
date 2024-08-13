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

	if (InTextureRHIRef->GetOwnerName() == NAME_None)
	{
		InTextureRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::UAVMask);

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

	if (InBufferRHIRef->GetOwnerName() == NAME_None)
	{
		InBufferRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::UAVMask);

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

	if (InBufferRHIRef->GetOwnerName() == NAME_None)
	{
		InBufferRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::UAVMask);

	return true;
}

FUnorderedAccessViewRHIRef UCompushadyUAV::GetRHI() const
{
	return UAVRHIRef;
}

void UCompushadyUAV::CopyToBuffer(UCompushadyUAV* DestinationBuffer, const int64 Size, const int64 DestinationOffset, const int64 SourceOffset, const FCompushadySignaled& OnSignaled)
{
	if (!DestinationBuffer)
	{
		OnSignaled.ExecuteIfBound(false, "Destination Buffer cannot be NULL");
		return;
	}

	if (this == DestinationBuffer)
	{
		OnSignaled.ExecuteIfBound(false, "Destination Buffer cannot be the Source one");
		return;
	}

	if (Size < 0 || DestinationOffset < 0 || SourceOffset < 0)
	{
		OnSignaled.ExecuteIfBound(false, "Size and Offsets cannot be negative");
		return;
	}

	if (!IsValidBuffer())
	{
		OnSignaled.ExecuteIfBound(false, "Invalid Source Buffer");
		return;
	}

	if (!DestinationBuffer->IsValidBuffer())
	{
		OnSignaled.ExecuteIfBound(false, "Invalid Destination Buffer");
		return;
	}

	int64 RequiredSize = Size;
	if (RequiredSize <= 0)
	{
		RequiredSize = GetBufferSize() - SourceOffset;
	}

	if (SourceOffset + RequiredSize > GetBufferSize())
	{
		OnSignaled.ExecuteIfBound(false, "Source Offset + Size out of bounds");
		return;
	}

	if (DestinationOffset + RequiredSize > DestinationBuffer->GetBufferSize())
	{
		OnSignaled.ExecuteIfBound(false, "Destination Offset + Size out of bounds");
		return;
	}

	EnqueueToGPU(
		[this, DestinationBuffer, RequiredSize, DestinationOffset, SourceOffset](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(DestinationBuffer->GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

			RHICmdList.CopyBufferRegion(DestinationBuffer->GetBufferRHI(), DestinationOffset, GetBufferRHI(), SourceOffset, RequiredSize);
		}, OnSignaled);
}

bool UCompushadyUAV::CopyToBufferSync(UCompushadyUAV* DestinationBuffer, const int64 Size, const int64 DestinationOffset, const int64 SourceOffset, FString& ErrorMessages)
{
	if (!DestinationBuffer)
	{
		ErrorMessages = "Destination Buffer cannot be NULL";
		return false;
	}

	if (this == DestinationBuffer)
	{
		ErrorMessages = "Destination Buffer cannot be the Source one";
		return false;
	}

	if (Size < 0 || DestinationOffset < 0 || SourceOffset < 0)
	{
		ErrorMessages = "Size and Offsets cannot be negative";
		return false;
	}

	if (!IsValidBuffer())
	{
		ErrorMessages = "Invalid Source Buffer";
		return false;
	}

	if (!DestinationBuffer->IsValidBuffer())
	{
		ErrorMessages = "Invalid Destination Buffer";
		return false;
	}

	int64 RequiredSize = Size;
	if (RequiredSize <= 0)
	{
		RequiredSize = GetBufferSize() - SourceOffset;
	}

	if (SourceOffset + RequiredSize > GetBufferSize())
	{
		ErrorMessages = "Source Offset + Size out of bounds";
		return false;
	}

	if (DestinationOffset + RequiredSize > DestinationBuffer->GetBufferSize())
	{
		ErrorMessages = "Destination Offset + Size out of bounds";
		return false;
	}

	EnqueueToGPUSync(
		[this, DestinationBuffer, RequiredSize, DestinationOffset, SourceOffset](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(DestinationBuffer->GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

			RHICmdList.CopyBufferRegion(DestinationBuffer->GetBufferRHI(), DestinationOffset, GetBufferRHI(), SourceOffset, RequiredSize);
		});

	return true;
}
