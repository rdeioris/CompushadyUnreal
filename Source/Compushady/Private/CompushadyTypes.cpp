// Copyright 2023 - Roberto De Ioris.


#include "CompushadyTypes.h"
#include "Serialization/ArrayWriter.h"

bool ICompushadySignalable::InitFence(UObject* InOwningObject)
{
	OwningObject = InOwningObject;

	FenceRHIRef = RHICreateGPUFence(TEXT("CompushadyFence"));
	if (!FenceRHIRef.IsValid() || !FenceRHIRef->IsValid())
	{
		return false;
	}

	return true;
}

void ICompushadySignalable::WriteFence(FRHICommandListImmediate& RHICmdList)
{
	RHICmdList.WriteGPUFence(FenceRHIRef);
}

void ICompushadySignalable::ClearFence()
{
	bRunning = true;
	FenceRHIRef->Clear();
}

void ICompushadySignalable::CheckFence(FCompushadySignaled OnSignal)
{
	if (!OwningObject.IsValid(false, true))
	{
		return;
	}

	if (!FenceRHIRef->Poll())
	{
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, OnSignal]() { CheckFence(OnSignal); });
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [this, OnSignal]()
			{
				bRunning = false;
		OnSignal.ExecuteIfBound(true, "");
		OnSignalReceived();
			});
	}
}

void ICompushadySignalable::CheckFence(FCompushadySignaledWithFloatPayload OnSignal, TArray<uint8>& Data)
{
	if (!OwningObject.IsValid(false, true))
	{
		return;
	}

	if (!FenceRHIRef->Poll())
	{
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, OnSignal, &Data]() { CheckFence(OnSignal, Data); });
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [this, OnSignal, &Data]()
			{
				float* ValuePtr = reinterpret_cast<float*>(Data.GetData());
		bRunning = false;
		OnSignal.ExecuteIfBound(true, *ValuePtr, "");
		OnSignalReceived();
			});
	}
}

void ICompushadySignalable::CheckFence(FCompushadySignaledWithFloatArrayPayload OnSignal, TArray<uint8>& Data)
{
	if (!OwningObject.IsValid(false, true))
	{
		return;
	}

	if (!FenceRHIRef->Poll())
	{
		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, OnSignal, &Data]() { CheckFence(OnSignal, Data); });
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [this, OnSignal, &Data]()
			{
				float* ValuesPtr = reinterpret_cast<float*>(Data.GetData());
		TArray<float> Values;
		Values.Append(ValuesPtr, Data.Num() / sizeof(float));
		bRunning = false;
		OnSignal.ExecuteIfBound(true, Values, "");
		OnSignalReceived();
			});
	}
}

FTextureRHIRef UCompushadyResource::GetTextureRHI() const
{
	return TextureRHIRef;
}

FBufferRHIRef UCompushadyResource::GetBufferRHI() const
{
	return BufferRHIRef;
}

const FRHITransitionInfo& UCompushadyResource::GetRHITransitionInfo() const
{
	return RHITransitionInfo;
}

FStagingBufferRHIRef UCompushadyResource::GetStagingBuffer()
{
	if (!StagingBufferRHIRef.IsValid())
	{
		StagingBufferRHIRef = RHICreateStagingBuffer();
	}
	return StagingBufferRHIRef;
}

void UCompushadyResource::ReadbackAllToFloatArray(const FCompushadySignaledWithFloatArrayPayload& OnSignaled)
{
	if (bRunning)
	{
		TArray<float> Values;
		OnSignaled.ExecuteIfBound(false, Values, "The UAV is already being processed by another task");
		return;
	}

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyReadback)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			FStagingBufferRHIRef StagingBuffer = GetStagingBuffer();
	RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBuffer, 0, BufferRHIRef->GetSize());
	RHICmdList.SubmitCommandsAndFlushGPU();
	RHICmdList.BlockUntilGPUIdle();
	uint8* Data = reinterpret_cast<uint8*>(RHICmdList.LockStagingBuffer(StagingBuffer, nullptr, 0, BufferRHIRef->GetSize()));
	ReadbackCache.Empty(BufferRHIRef->GetSize());
	ReadbackCache.Append(Data, (BufferRHIRef->GetSize() / sizeof(float)) * sizeof(float));
	RHICmdList.UnlockStagingBuffer(StagingBuffer);
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled, ReadbackCache);
}

void UCompushadyResource::ReadbackAllToFile(const FString& Filename, const FCompushadySignaled& OnSignaled)
{
	if (bRunning)
	{
		TArray<float> Values;
		OnSignaled.ExecuteIfBound(false, "The UAV is already being processed by another task");
		return;
	}

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyReadback)(
		[this, Filename](FRHICommandListImmediate& RHICmdList)
		{
			FStagingBufferRHIRef StagingBuffer = GetStagingBuffer();
	RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBuffer, 0, BufferRHIRef->GetSize());
	RHICmdList.SubmitCommandsAndFlushGPU();
	RHICmdList.BlockUntilGPUIdle();
	uint8* Data = reinterpret_cast<uint8*>(RHICmdList.LockStagingBuffer(StagingBuffer, nullptr, 0, BufferRHIRef->GetSize()));
	ReadbackCache.Empty(BufferRHIRef->GetSize());
	ReadbackCache.Append(Data, (BufferRHIRef->GetSize() / sizeof(float)) * sizeof(float));
	FFileHelper::SaveArrayToFile(ReadbackCache, *Filename);
	RHICmdList.UnlockStagingBuffer(StagingBuffer);
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

void UCompushadyResource::ReadbackToFloatArray(const int32 Offset, const int32 Elements, const FCompushadySignaledWithFloatArrayPayload& OnSignaled)
{
	if (bRunning)
	{
		TArray<float> Values;
		OnSignaled.ExecuteIfBound(false, Values, "The UAV is already being processed by another task");
		return;
	}

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyReadback)(
		[this, Offset, Elements](FRHICommandListImmediate& RHICmdList)
		{
			FStagingBufferRHIRef StagingBuffer = GetStagingBuffer();
	RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBuffer, 0, BufferRHIRef->GetSize());
	RHICmdList.SubmitCommandsAndFlushGPU();
	RHICmdList.BlockUntilGPUIdle();
	uint8* Data = reinterpret_cast<uint8*>(RHICmdList.LockStagingBuffer(StagingBuffer, nullptr, 0, BufferRHIRef->GetSize()));
	ReadbackCache.Empty(Elements * sizeof(float));
	ReadbackCache.Append(Data + sizeof(float) * Offset, Elements * sizeof(float));
	RHICmdList.UnlockStagingBuffer(StagingBuffer);
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled, ReadbackCache);
}

void UCompushadyResource::CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled)
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
	RHICmdList.CopyTexture(TextureRHIRef, Resource->GetTextureRHI(), CopyTextureInfo);
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

void UCompushadyResource::CopyToRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray, const int32 Slice, const FCompushadySignaled& OnSignaled)
{
	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "The UAV is already being processed by another task");
		return;
	}

	if (!RenderTargetArray->GetResource() || !RenderTargetArray->GetResource()->IsInitialized())
	{
		RenderTargetArray->UpdateResource();
		RenderTargetArray->UpdateResourceImmediate(false);
	}

	FTextureResource* Resource = RenderTargetArray->GetResource();

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyCopyToRenderTarget2D)(
		[this, Resource, Slice](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.Transition(FRHITransitionInfo(Resource->GetTextureRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));
	FRHICopyTextureInfo CopyTextureInfo;
	CopyTextureInfo.DestSliceIndex = Slice;
	RHICmdList.CopyTexture(TextureRHIRef, Resource->GetTextureRHI(), CopyTextureInfo);
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

void UCompushadyResource::OnSignalReceived()
{

}