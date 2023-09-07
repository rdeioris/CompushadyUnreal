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

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateUnorderedAccessView)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			UAVRHIRef = COMPUSHADY_CREATE_UAV(TextureRHIRef);
		});

	FlushRenderingCommands();

	if (!UAVRHIRef)
	{
		return false;
	}

	if (!InitFence(this))
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

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateUnorderedAccessView)(
		[this, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			UAVRHIRef = COMPUSHADY_CREATE_UAV(BufferRHIRef, static_cast<uint8>(PixelFormat));
		});

	FlushRenderingCommands();

	if (!UAVRHIRef)
	{
		return false;
	}

	if (!InitFence(this))
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

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateUnorderedAccessView)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			UAVRHIRef = COMPUSHADY_CREATE_UAV(BufferRHIRef, false, false);
		});

	FlushRenderingCommands();

	if (!UAVRHIRef)
	{
		return false;
	}

	if (!InitFence(this))
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

void UCompushadyUAV::CopyToStaticMeshTexCoords(UStaticMesh* StaticMesh, const int32 LOD, const FCompushadySignaled& OnSignaled)
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

	FBufferRHIRef DestBufferRHIRef = LODResources.VertexBuffers.StaticMeshVertexBuffer.TexCoordVertexBuffer.VertexBufferRHI;
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
	ENQUEUE_RENDER_COMMAND(DoCompushadyCopyFromRHIBuffer)(
		[this, SourceBufferRHIRef](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(SourceBufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

			RHICmdList.CopyBufferRegion(GetBufferRHI(), 0, SourceBufferRHIRef, 0, SourceBufferRHIRef->GetSize());
		});

	FlushRenderingCommands();
	return true;
}