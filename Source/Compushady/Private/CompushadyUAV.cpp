// Copyright 2023 - Roberto De Ioris.


#include "CompushadyUAV.h"

bool UCompushadyUAV::InitializeFromTexture(FTextureRHIRef InTextureRHIRef)
{
	if (!InTextureRHIRef)
	{
		return false;
	}

	TextureRHIRef = InTextureRHIRef;

	UAVRHIRef = RHICreateUnorderedAccessView(TextureRHIRef);
	if (!UAVRHIRef)
	{
		return false;
	}

	RHITransitionInfo = FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::UAVCompute);

	return true;
}

bool UCompushadyUAV::InitializeFromBuffer(FBufferRHIRef InBufferRHIRef)
{
	if (!InBufferRHIRef)
	{
		return false;
	}

	BufferRHIRef = InBufferRHIRef;

	UAVRHIRef = RHICreateUnorderedAccessView(BufferRHIRef, false, false);
	if (!UAVRHIRef)
	{
		return false;
	}

	RHITransitionInfo = FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::UAVCompute);

	return true;
}

FUnorderedAccessViewRHIRef UCompushadyUAV::GetRHI() const
{
	return UAVRHIRef;
}

void UCompushadyUAV::Readback()
{
	if (!StagingBufferRHIRef.IsValid())
	{
		StagingBufferRHIRef = RHICreateStagingBuffer();
	}

	ENQUEUE_RENDER_COMMAND(DoCompushadyReadback)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBufferRHIRef, 0, BufferRHIRef->GetSize());
	RHICmdList.SubmitCommandsAndFlushGPU();
	RHICmdList.BlockUntilGPUIdle();
	uint32* Data = (uint32*)RHICmdList.LockStagingBuffer(StagingBufferRHIRef, nullptr, 0, BufferRHIRef->GetSize());
	UE_LOG(LogTemp, Error, TEXT("%u %u %u"), Data[0], Data[1], Data[2]);
	RHICmdList.UnlockStagingBuffer(StagingBufferRHIRef);
		});
}

void UCompushadyUAV::CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget)
{
	if (!RenderTarget->GetResource() || !RenderTarget->GetResource()->IsInitialized())
	{
		RenderTarget->UpdateResource();
		RenderTarget->UpdateResourceImmediate(false);
	}

	FTextureResource* Resource = RenderTarget->GetResource();

	ENQUEUE_RENDER_COMMAND(DoCompushadyCopyToRenderTarget2D)(
		[this, Resource](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.Transition(FRHITransitionInfo(Resource->GetTextureRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));
	FRHICopyTextureInfo CopyTextureInfo;
	CopyTextureInfo.Size = FIntVector(1024, 1024, 1);

	RHICmdList.CopyTexture(TextureRHIRef, Resource->GetTextureRHI(), CopyTextureInfo);
		});
}

FTextureRHIRef UCompushadyUAV::GetTextureRHI() const
{
	return TextureRHIRef;
}

FBufferRHIRef UCompushadyUAV::GetBufferRHI() const
{
	return BufferRHIRef;
}

const FRHITransitionInfo& UCompushadyUAV::GetRHITransitionInfo() const
{
	return RHITransitionInfo;
}