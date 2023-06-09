// Copyright 2023 - Roberto De Ioris.


#include "CompushadyTypes.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
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

FTextureRHIRef UCompushadyResource::GetReadbackTexture()
{
	if (!ReadbackTextureRHIRef.IsValid())
	{
		FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(nullptr, TextureRHIRef->GetSizeX(), TextureRHIRef->GetSizeY(), TextureRHIRef->GetFormat());
		TextureCreateDesc.SetFlags(ETextureCreateFlags::CPUReadback);
		ReadbackTextureRHIRef = RHICreateTexture(TextureCreateDesc);
	}
	return ReadbackTextureRHIRef;
}

void UCompushadyResource::ReadbackAllToFloatArray(const FCompushadySignaledWithFloatArrayPayload& OnSignaled)
{
	if (bRunning)
	{
		TArray<float> Values;
		OnSignaled.ExecuteIfBound(false, Values, "The Resource is already being processed by another task");
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

void UCompushadyResource::ReadbackTextureToPngFile(const FString& Filename, const FCompushadySignaled& OnSignaled)
{
	auto PngWriter = [this, Filename](void* Data)
	{
		FIntVector Size = GetTextureSize();
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (ImageWrapper.IsValid())
		{

			if (ImageWrapper->SetRaw(Data, Size.X * Size.Y * GPixelFormats[GetTextureRHI()->GetDesc().Format].BlockBytes, Size.X, Size.Y, ERGBFormat::BGRA, 8))
			{

				TArray64<uint8> CompressedBytes = ImageWrapper->GetCompressed();
				FFileHelper::SaveArrayToFile(CompressedBytes, *Filename);
			}
		}
	};

	MapAndExecute(PngWriter, OnSignaled);
}

void UCompushadyResource::ReadbackAllToFile(const FString& Filename, const FCompushadySignaled& OnSignaled)
{
	if (bRunning)
	{
		TArray<float> Values;
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
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
		OnSignaled.ExecuteIfBound(false, Values, "The Resource is already being processed by another task");
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

void UCompushadyResource::CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled, const FCompushadyCopyInfo& CopyInfo)
{
	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
		return;
	}

	if (!RenderTarget->GetResource() || !RenderTarget->GetResource()->IsInitialized())
	{
		RenderTarget->UpdateResource();
		RenderTarget->UpdateResourceImmediate(false);
	}

	FTextureResource* Resource = RenderTarget->GetResource();

	if (!Resource)
	{
		OnSignaled.ExecuteIfBound(false, "RenderTarget2D Resource not available");
		return;
	}

	if (!CopyTexture_Internal(Resource->GetTextureRHI(), TextureRHIRef, CopyInfo, OnSignaled))
	{
		return;
	}

	CheckFence(OnSignaled);
}

void UCompushadyResource::CopyFromMediaTexture(UMediaTexture* MediaTexture, const FCompushadySignaled& OnSignaled, const FCompushadyCopyInfo& CopyInfo)
{
	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
		return;
	}

	if (!MediaTexture)
	{
		OnSignaled.ExecuteIfBound(false, "MediaTexture is NULL");
		return;
	}

	FTextureResource* Resource = MediaTexture->GetResource();

	if (!Resource)
	{
		OnSignaled.ExecuteIfBound(false, "MediaTexture Resource not available");
		return;
	}

	if (!CopyTexture_Internal(TextureRHIRef, Resource->GetTextureRHI(), CopyInfo, OnSignaled))
	{
		return;
	}

	CheckFence(OnSignaled);
}


void UCompushadyResource::CopyToRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray, const FCompushadySignaled& OnSignaled, const FCompushadyCopyInfo& CopyInfo)
{
	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
		return;
	}

	if (!RenderTargetArray->GetResource() || !RenderTargetArray->GetResource()->IsInitialized())
	{
		RenderTargetArray->UpdateResource();
		RenderTargetArray->UpdateResourceImmediate(false);
	}

	FTextureResource* Resource = RenderTargetArray->GetResource();

	if (!Resource)
	{
		OnSignaled.ExecuteIfBound(false, "RenderTarget2DArray Resource not available");
		return;
	}

	if (!CopyTexture_Internal(Resource->GetTextureRHI(), TextureRHIRef, CopyInfo, OnSignaled))
	{
		return;
	}

	CheckFence(OnSignaled);
}

bool ICompushadySignalable::CopyTexture_Internal(FTextureRHIRef Destination, FTextureRHIRef Source, const FCompushadyCopyInfo& CopyInfo, const FCompushadySignaled& OnSignaled)
{
	if (Destination->GetFormat() != Source->GetFormat())
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Incompatible Texture Formats (%s vs %s)"),
			GetPixelFormatString(Destination->GetFormat()),
			GetPixelFormatString(Source->GetFormat())
		));
		return false;
	}

	FIntVector DestinationSize = Destination->GetSizeXYZ();
	FIntVector SourceSize = Source->GetSizeXYZ();

	const FRHITextureDesc& DestinationDesc = Destination->GetDesc();
	int32 DestinationNumSlices = DestinationDesc.ArraySize;
	if (DestinationDesc.IsTextureCube())
	{
		DestinationNumSlices *= 6;
	}

	const FRHITextureDesc& SourceDesc = Source->GetDesc();
	int32 SourceNumSlices = SourceDesc.ArraySize;
	if (SourceDesc.IsTextureCube())
	{
		SourceNumSlices *= 6;
	}

	FRHICopyTextureInfo CopyTextureInfo;
	CopyTextureInfo.Size = CopyInfo.SourceSize;

	if (CopyTextureInfo.Size.X == 0)
	{
		CopyTextureInfo.Size.X = SourceSize.X;
	}

	if (CopyTextureInfo.Size.Y == 0)
	{
		CopyTextureInfo.Size.Y = SourceSize.Y;
	}

	if (CopyTextureInfo.Size.Z == 0)
	{
		CopyTextureInfo.Size.Z = SourceSize.Z;
	}

	CopyTextureInfo.DestPosition = CopyInfo.DestinationOffset;
	CopyTextureInfo.SourcePosition = CopyInfo.SourceOffset;

	if (CopyTextureInfo.SourcePosition.X < 0 || CopyTextureInfo.SourcePosition.X >= SourceSize.X ||
		CopyTextureInfo.SourcePosition.Y < 0 || CopyTextureInfo.SourcePosition.Y >= SourceSize.Y ||
		CopyTextureInfo.SourcePosition.Z < 0 || CopyTextureInfo.SourcePosition.Z >= SourceSize.Z)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid Texture Source Offset (%s)"),
			*CopyTextureInfo.SourcePosition.ToString())
		);
		return false;
	}

	if (CopyTextureInfo.SourcePosition.X + CopyTextureInfo.Size.X >= SourceSize.X)
	{
		CopyTextureInfo.Size.X = SourceSize.X - CopyTextureInfo.SourcePosition.X;
	}

	if (CopyTextureInfo.SourcePosition.Y + CopyTextureInfo.Size.Y >= SourceSize.Y)
	{
		CopyTextureInfo.Size.Y = SourceSize.Y - CopyTextureInfo.SourcePosition.Y;
	}

	if (CopyTextureInfo.SourcePosition.Z + CopyTextureInfo.Size.Z >= SourceSize.Z)
	{
		CopyTextureInfo.Size.Z = SourceSize.Z - CopyTextureInfo.SourcePosition.Z;
	}

	if (CopyTextureInfo.DestPosition.X < 0 || CopyTextureInfo.DestPosition.X >= DestinationSize.X ||
		CopyTextureInfo.DestPosition.Y < 0 || CopyTextureInfo.DestPosition.Y >= DestinationSize.Y ||
		CopyTextureInfo.DestPosition.Z < 0 || CopyTextureInfo.DestPosition.Z >= DestinationSize.Z)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid Texture Destination Offset (%s)"),
			*CopyTextureInfo.DestPosition.ToString())
		);
		return false;
	}

	if (CopyTextureInfo.DestPosition.X + CopyTextureInfo.Size.X >= DestinationSize.X)
	{
		CopyTextureInfo.Size.X = DestinationSize.X - CopyTextureInfo.DestPosition.X;
	}

	if (CopyTextureInfo.DestPosition.Y + CopyTextureInfo.Size.Y >= DestinationSize.Y)
	{
		CopyTextureInfo.Size.Y = DestinationSize.Y - CopyTextureInfo.DestPosition.Y;
	}

	if (CopyTextureInfo.DestPosition.Z + CopyTextureInfo.Size.Z >= DestinationSize.Z)
	{
		CopyTextureInfo.Size.Z = DestinationSize.Z - CopyTextureInfo.DestPosition.Z;
	}

	CopyTextureInfo.SourceSliceIndex = CopyInfo.SourceSlice;
	if (static_cast<int32>(CopyTextureInfo.SourceSliceIndex) >= SourceNumSlices)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid Texture Source Slice Index (%u)"),
			CopyTextureInfo.SourceSliceIndex)
		);
		return false;
	}

	CopyTextureInfo.DestSliceIndex = CopyInfo.DestinationSlice;
	if (static_cast<int32>(CopyTextureInfo.DestSliceIndex) >= DestinationNumSlices)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid Texture Destination Slice Index (%u)"),
			CopyTextureInfo.SourceSliceIndex)
		);
		return false;
	}

	CopyTextureInfo.NumSlices = CopyInfo.NumSlices;
	if (CopyTextureInfo.NumSlices == 0 || CopyTextureInfo.SourceSliceIndex + CopyTextureInfo.NumSlices > static_cast<uint32>(SourceNumSlices) || CopyTextureInfo.DestSliceIndex + CopyTextureInfo.NumSlices > static_cast<uint32>(DestinationNumSlices))
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid Texture Number Of Slices for Copy (%u)"),
			CopyTextureInfo.NumSlices)
		);
		return false;
	}

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyCopyToRenderTarget2D)(
		[this, Destination, Source, CopyTextureInfo](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(Source, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	RHICmdList.Transition(FRHITransitionInfo(Destination, ERHIAccess::Unknown, ERHIAccess::CopyDest));

	RHICmdList.CopyTexture(Source, Destination, CopyTextureInfo);
	WriteFence(RHICmdList);
		});

	return true;
}

void UCompushadyResource::OnSignalReceived()
{

}

FIntVector UCompushadyResource::GetTextureThreadGroupSize(const FIntVector XYZ, const bool bUseNumSlicesForZ) const
{
	if (TextureRHIRef.IsValid())
	{
		if (XYZ.X <= 0 || XYZ.Y <= 0 || XYZ.Z <= 0)
		{
			return FIntVector(1, 1, 1);
		}

		FIntVector TextureXYZ = TextureRHIRef->GetSizeXYZ();
		return FIntVector(
			FMath::DivideAndRoundUp(TextureXYZ.X, XYZ.X),
			FMath::DivideAndRoundUp(TextureXYZ.Y, XYZ.Y),
			FMath::DivideAndRoundUp(bUseNumSlicesForZ ? GetTextureNumSlices() : TextureXYZ.Z, XYZ.Z)
		);
	}
	return XYZ;

}

FIntVector UCompushadyResource::GetTextureSize() const
{
	if (TextureRHIRef.IsValid())
	{
		return TextureRHIRef->GetSizeXYZ();
	}

	return FIntVector(0, 0, 0);
}

int32 UCompushadyResource::GetTextureNumSlices() const
{
	if (TextureRHIRef.IsValid())
	{
		const FRHITextureDesc& Desc = TextureRHIRef->GetDesc();
		if (Desc.IsTextureCube())
		{
			return Desc.ArraySize * 6;
		}
		return Desc.ArraySize;
	}
	return 0;
}

void UCompushadyResource::MapAndExecute(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled)
{
	if (bRunning)
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
		return;
	}

	ClearFence();

	ENQUEUE_RENDER_COMMAND(DoCompushadyReadbackTexture)(
		[this, InFunction](FRHICommandListImmediate& RHICmdList)
		{
			FTextureRHIRef ReadbackTexture = GetReadbackTexture();
	RHICmdList.Transition(FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
	FRHICopyTextureInfo CopyTextureInfo;
	RHICmdList.CopyTexture(TextureRHIRef, ReadbackTexture, CopyTextureInfo);
	RHICmdList.SubmitCommandsAndFlushGPU();
	RHICmdList.BlockUntilGPUIdle();
	int32 Width = 0;
	int32 Height = 0;
	void* Data = nullptr;
	RHICmdList.MapStagingSurface(ReadbackTexture, Data, Width, Height);
	if (Data)
	{
		InFunction(Data);
		RHICmdList.UnmapStagingSurface(GetTextureRHI());
	}
	WriteFence(RHICmdList);
		});

	CheckFence(OnSignaled);
}

void UCompushadyResource::MapAndExecuteInGameThread(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled)
{
	auto Wrapper = [this, InFunction](void* Data)
	{
		FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, InFunction, Data]()
			{
				InFunction(Data);
			}, TStatId(), nullptr, ENamedThreads::GameThread);
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(Task);
	};

	MapAndExecute(Wrapper, OnSignaled);
}