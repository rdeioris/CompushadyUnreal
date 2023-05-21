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

	UAVRHIRef = RHICreateUnorderedAccessView(TextureRHIRef);
	if (!UAVRHIRef)
	{
		return false;
	}

	if (!InitFence())
	{
		return false;
	}

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

	UAVRHIRef = RHICreateUnorderedAccessView(BufferRHIRef, static_cast<uint8>(PixelFormat));
	if (!UAVRHIRef)
	{
		return false;
	}

	if (!InitFence())
	{
		return false;
	}

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

	UAVRHIRef = RHICreateUnorderedAccessView(BufferRHIRef, false, false);
	if (!UAVRHIRef)
	{
		return false;
	}

	if (!InitFence())
	{
		return false;
	}

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

	RHICmdList.UnlockStagingBuffer(StagingBufferRHIRef);
		});
}

void UCompushadyUAV::CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled)
{
	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "The UAV is already being processed by another task");
		return;
	}

	if (!RenderTarget->GetResource() || !RenderTarget->GetResource()->IsInitialized())
	{
		RenderTarget->UpdateResource();
		RenderTarget->UpdateResourceImmediate(false);
	}

	FTextureResource* Resource = RenderTarget->GetResource();

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyCopyToRenderTarget2D)(
		[this, Resource](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.Transition(FRHITransitionInfo(Resource->GetTextureRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));
	FRHICopyTextureInfo CopyTextureInfo;
	CopyTextureInfo.Size = FIntVector(1024, 1024, 1);

	RHICmdList.CopyTexture(TextureRHIRef, Resource->GetTextureRHI(), CopyTextureInfo);
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

void UCompushadyUAV::CopyToSRV(UCompushadySRV* SRV, const FCompushadySignaled& OnSignaled)
{
	if (!SRV)
	{
		OnSignaled.ExecuteIfBound(false, "SRV is NULL");
		return;
	}

	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "The UAV is already being processed by another task");
		return;
	}

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyCopyToRenderTarget2D)(
		[this, SRV](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.Transition(FRHITransitionInfo(SRV->GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

	RHICmdList.CopyBufferRegion(SRV->GetBufferRHI(), 0, BufferRHIRef, 0, SRV->GetBufferRHI()->GetSize());
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

void UCompushadyUAV::CopyToStaticMeshPositions(UStaticMesh* StaticMesh, const int32 LOD, const FCompushadySignaled& OnSignaled)
{
	if (!StaticMesh)
	{
		OnSignaled.ExecuteIfBound(false, "StaticMesh is NULL");
		return;
	}

	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "The UAV is already being processed by another task");
		return;
	}

	if (!StaticMesh->AreRenderingResourcesInitialized())
	{
		StaticMesh->InitResources();
	}

	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();

	if (LOD >= RenderData->LODResources.Num())
	{
		OnSignaled.ExecuteIfBound(false, "Invalid LOD");
		return;
	}

	FStaticMeshLODResources& LODResources = RenderData->LODResources[LOD];

	FBufferRHIRef DestBufferRHIRef = LODResources.VertexBuffers.PositionVertexBuffer.VertexBufferRHI;
	if (!DestBufferRHIRef.IsValid() || !DestBufferRHIRef->IsValid())
	{
		OnSignaled.ExecuteIfBound(false, "Invalid RHI Buffer");
		return;
	}

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyCopyToBuffer)(
		[this, DestBufferRHIRef](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.Transition(FRHITransitionInfo(DestBufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopyDest));

	RHICmdList.CopyBufferRegion(DestBufferRHIRef, 0, GetBufferRHI(), 0, GetBufferRHI()->GetSize());
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

bool UCompushadyUAV::CopyFromRHIBuffer(FBufferRHIRef SourceBufferRHIRef)
{
	ENQUEUE_RENDER_COMMAND(DoCompushadyCopyToFromRHIBuffer)(
		[this, SourceBufferRHIRef](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(SourceBufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

	RHICmdList.CopyBufferRegion(GetBufferRHI(), 0, SourceBufferRHIRef, 0, SourceBufferRHIRef->GetSize());
		});

	FlushRenderingCommands();
	return true;
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