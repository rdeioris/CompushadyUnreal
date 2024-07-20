// Copyright 2023 - Roberto De Ioris.


#include "CompushadyTypes.h"
#include "CompushadyCBV.h"
#include "CompushadySampler.h"
#include "CompushadySRV.h"
#include "CompushadyUAV.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Serialization/ArrayWriter.h"

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
	if (!StagingBufferRHIRef.IsValid() || !StagingBufferRHIRef->IsValid())
	{
		StagingBufferRHIRef = RHICreateStagingBuffer();
	}
	return StagingBufferRHIRef;
}

FBufferRHIRef UCompushadyResource::GetUploadBuffer(FRHICommandListImmediate& RHICmdList)
{
	if (!UploadBufferRHIRef.IsValid() || !UploadBufferRHIRef->IsValid())
	{
		FRHIResourceCreateInfo ResourceCreateInfo(TEXT(""));
		UploadBufferRHIRef = COMPUSHADY_CREATE_BUFFER(BufferRHIRef->GetSize(), EBufferUsageFlags::VertexBuffer, BufferRHIRef->GetStride(), ERHIAccess::CopySrc, ResourceCreateInfo);
	}

	return UploadBufferRHIRef;
}

FTextureRHIRef UCompushadyResource::GetReadbackTexture()
{
	if (!ReadbackTextureRHIRef.IsValid() || !ReadbackTextureRHIRef->IsValid())
	{
		FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(nullptr, TextureRHIRef->GetSizeX(), TextureRHIRef->GetSizeY(), TextureRHIRef->GetFormat());
		TextureCreateDesc.SetFlags(ETextureCreateFlags::CPUReadback);
		ReadbackTextureRHIRef = RHICreateTexture(TextureCreateDesc);
	}
	return ReadbackTextureRHIRef;
}

bool UCompushadyResource::UpdateTextureSliceSync(const uint8* Ptr, const int64 Size, const int32 Slice)
{
	if (IsRunning() || !IsValidTexture())
	{
		return false;
	}

	const int32 RowPitch = TextureRHIRef->GetSizeX() * GPixelFormats[TextureRHIRef->GetFormat()].BlockBytes;

	ENQUEUE_RENDER_COMMAND(DoCompushadyUpdateTexture)(
		[this, Ptr, Size, Slice, RowPitch](FRHICommandListImmediate& RHICmdList)
		{
			const ETextureDimension Dimension = TextureRHIRef->GetDesc().Dimension;
			if (Dimension == ETextureDimension::Texture2D)
			{
				uint32 DestStride;
				void* Data = RHICmdList.LockTexture2D(TextureRHIRef, 0, EResourceLockMode::RLM_WriteOnly, DestStride, false);
				if (Data)
				{
					CopyTextureData2D(Ptr, Data, TextureRHIRef->GetSizeY(), TextureRHIRef->GetFormat(), RowPitch, DestStride);
					RHICmdList.UnlockTexture2D(TextureRHIRef, 0, false);
				}
			}
			else if (Dimension == ETextureDimension::Texture2DArray)
			{
				uint32 DestStride;
				void* Data = RHICmdList.LockTexture2DArray(TextureRHIRef, Slice, 0, EResourceLockMode::RLM_WriteOnly, DestStride, false);
				if (Data)
				{
					CopyTextureData2D(Ptr, Data, TextureRHIRef->GetSizeY(), TextureRHIRef->GetFormat(), RowPitch, DestStride);
					RHICmdList.UnlockTexture2DArray(TextureRHIRef, Slice, 0, false);
				}
			}
			else if (Dimension == ETextureDimension::Texture3D)
			{
				FUpdateTextureRegion3D UpdateRegion(0, 0, Slice, 0, 0, 0,
					TextureRHIRef->GetSizeX(),
					TextureRHIRef->GetSizeY(),
					1);
				RHICmdList.UpdateTexture3D(TextureRHIRef, 0, UpdateRegion, RowPitch, RowPitch * TextureRHIRef->GetSizeY(), Ptr);
			}
		});

	FlushRenderingCommands();

	return true;
}

bool UCompushadyResource::UpdateTextureSliceSync(const TArray<uint8>& Pixels, const int32 Slice)
{
	return UpdateTextureSliceSync(Pixels.GetData(), Pixels.Num(), Slice);
}

void UCompushadyResource::ReadbackAllToFloatArray(const FCompushadySignaledWithFloatArrayPayload& OnSignaled)
{
	if (IsRunning())
	{
		TArray<float> Values;
		OnSignaled.ExecuteIfBound(false, Values, "The Resource is already being processed by another task");
		return;
	}

	EnqueueToGPU(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			FStagingBufferRHIRef StagingBuffer = GetStagingBuffer();
			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBuffer, 0, BufferRHIRef->GetSize());
			WaitForGPU(RHICmdList);
			uint8* Data = reinterpret_cast<uint8*>(RHICmdList.LockStagingBuffer(StagingBuffer, nullptr, 0, BufferRHIRef->GetSize()));
			const uint32 FloatBufferSize = BufferRHIRef->GetSize() / sizeof(float);
			ReadbackCacheFloats.Empty(FloatBufferSize);
			ReadbackCacheFloats.Append(reinterpret_cast<const float*>(Data), FloatBufferSize);
			RHICmdList.UnlockStagingBuffer(StagingBuffer);
		}, OnSignaled, ReadbackCacheFloats);
}

bool UCompushadyResource::IsValidTexture() const
{
	return TextureRHIRef.IsValid() && TextureRHIRef->IsValid();
}

bool UCompushadyResource::IsValidBuffer() const
{
	return BufferRHIRef.IsValid() && BufferRHIRef->IsValid();
}

EPixelFormat UCompushadyResource::GetTexturePixelFormat() const
{
	if (IsValidTexture())
	{
		return TextureRHIRef->GetDesc().Format;
	}

	return EPixelFormat::PF_Unknown;
}

void UCompushadyResource::ReadbackTextureToPngFile(const FString& Filename, const FCompushadySignaled& OnSignaled)
{
	if (!IsValidTexture())
	{
		OnSignaled.ExecuteIfBound(false, "The resource is not a valid Texture");
		return;
	}

	auto PngWriter = [this, Filename](const void* Data)
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

	MapReadAndExecute(PngWriter, OnSignaled);
}

void UCompushadyResource::ReadbackAllToFile(const FString& Filename, const FCompushadySignaled& OnSignaled)
{
	auto FileWriter = [this, Filename](const void* Data)
		{
			int32 Size = 0;
			if (IsValidBuffer())
			{
				Size = GetBufferSize();
			}
			else if (IsValidTexture())
			{
				Size = TextureRHIRef->GetDesc().CalcMemorySizeEstimate();
			}
			if (Size > 0)
			{
				TArrayView<const uint8> ArrayView = TArrayView<const uint8>(reinterpret_cast<const uint8*>(Data), Size);
				FFileHelper::SaveArrayToFile(ArrayView, *Filename);
			}
		};

	MapReadAndExecute(FileWriter, OnSignaled);
}

void UCompushadyResource::ReadbackToFloatArray(const int32 Offset, const int32 Elements, const FCompushadySignaledWithFloatArrayPayload& OnSignaled)
{
	if (IsRunning())
	{
		TArray<float> Values;
		OnSignaled.ExecuteIfBound(false, Values, "The Resource is already being processed by another task");
		return;
	}

	EnqueueToGPU(
		[this, Offset, Elements](FRHICommandListImmediate& RHICmdList)
		{
			FStagingBufferRHIRef StagingBuffer = GetStagingBuffer();
			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBuffer, 0, BufferRHIRef->GetSize());
			WaitForGPU(RHICmdList);
			uint8* Data = reinterpret_cast<uint8*>(RHICmdList.LockStagingBuffer(StagingBuffer, nullptr, 0, BufferRHIRef->GetSize()));
			ReadbackCacheFloats.Empty(Elements);
			ReadbackCacheFloats.Append(reinterpret_cast<const float*>(Data) + Offset, Elements);
			RHICmdList.UnlockStagingBuffer(StagingBuffer);
		}, OnSignaled, ReadbackCacheFloats);
}

void UCompushadyResource::CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
		return;
	}

	if (!RenderTarget->GetResource() || !RenderTarget->GetResource()->IsInitialized())
	{
		RenderTarget->UpdateResource();
		RenderTarget->UpdateResourceImmediate(false);
		FlushRenderingCommands();
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
}

void UCompushadyResource::CopyFromMediaTexture(UMediaTexture* MediaTexture, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo)
{
	if (IsRunning())
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
}


void UCompushadyResource::CopyToRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
		return;
	}

	if (!RenderTargetArray->GetResource() || !RenderTargetArray->GetResource()->IsInitialized())
	{
		RenderTargetArray->UpdateResource();
		RenderTargetArray->UpdateResourceImmediate(false);
		FlushRenderingCommands();
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
}

bool ICompushadySignalable::CopyTexture_Internal(FTextureRHIRef Destination, FTextureRHIRef Source, const FCompushadyTextureCopyInfo& CopyInfo, const FCompushadySignaled& OnSignaled)
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

	EnqueueToGPU(
		[this, Destination, Source, CopyTextureInfo](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.Transition(FRHITransitionInfo(Source, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(Destination, ERHIAccess::Unknown, ERHIAccess::CopyDest));

			RHICmdList.CopyTexture(Source, Destination, CopyTextureInfo);
		}, OnSignaled);

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
	if (TextureRHIRef.IsValid() && TextureRHIRef->IsValid())
	{
		return TextureRHIRef->GetSizeXYZ();
	}

	return FIntVector(0, 0, 0);
}

int64 UCompushadyResource::GetBufferSize() const
{
	if (BufferRHIRef.IsValid() && BufferRHIRef->IsValid())
	{
		return static_cast<int64>(BufferRHIRef->GetSize());
	}
	return 0;
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

void UCompushadyResource::MapReadAndExecute(TFunction<void(const void*)> InFunction, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
		return;
	}

	if (IsValidBuffer())
	{
		ENQUEUE_RENDER_COMMAND(DoCompushadyReadbackBuffer)(
			[this, InFunction](FRHICommandListImmediate& RHICmdList)
			{
				FStagingBufferRHIRef StagingBuffer = GetStagingBuffer();
				RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
				RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBuffer, 0, BufferRHIRef->GetSize());
				WaitForGPU(RHICmdList);
				void* Data = RHICmdList.LockStagingBuffer(StagingBuffer, nullptr, 0, BufferRHIRef->GetSize());
				if (Data)
				{
					InFunction(Data);
					RHICmdList.UnlockStagingBuffer(StagingBuffer);
				}
				WaitForGPU(RHICmdList);
			});
	}
	else
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is in invalid state or is not mappable");
		return;
	}

	BeginFence(OnSignaled);
}

void UCompushadyResource::MapWriteAndExecute(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is already being processed by another task");
		return;
	}

	if (IsValidBuffer())
	{
		EnqueueToGPU(
			[this, InFunction](FRHICommandListImmediate& RHICmdList)
			{
				FBufferRHIRef UploadBuffer = GetUploadBuffer(RHICmdList);
				void* Data = RHICmdList.LockBuffer(UploadBuffer, 0, UploadBuffer->GetSize(), EResourceLockMode::RLM_WriteOnly);
				if (Data)
				{
					InFunction(Data);
					RHICmdList.UnlockBuffer(UploadBuffer);
				}
				RHICmdList.Transition(FRHITransitionInfo(UploadBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc));
				RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopyDest));
				RHICmdList.CopyBufferRegion(BufferRHIRef, 0, UploadBuffer, 0, UploadBuffer->GetSize());
			}, OnSignaled);
	}
	else
	{
		OnSignaled.ExecuteIfBound(false, "The Resource is in invalid state or is not mappable");
		return;
	}
}

bool UCompushadyResource::MapReadAndExecuteSync(TFunction<void(const void*)> InFunction)
{
	if (IsRunning())
	{
		return false;
	}

	if (!IsValidBuffer())
	{
		return false;
	}

	EnqueueToGPUSync(
		[this, InFunction](FRHICommandListImmediate& RHICmdList)
		{
			FStagingBufferRHIRef StagingBuffer = GetStagingBuffer();
			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBuffer, 0, BufferRHIRef->GetSize());
			WaitForGPU(RHICmdList);
			void* Data = RHICmdList.LockStagingBuffer(StagingBuffer, nullptr, 0, BufferRHIRef->GetSize());
			if (Data)
			{
				InFunction(Data);
				RHICmdList.UnlockStagingBuffer(StagingBuffer);
			}
		});

	return true;
}

bool UCompushadyResource::MapWriteAndExecuteSync(TFunction<void(void*)> InFunction)
{
	if (IsRunning())
	{
		return false;
	}

	if (!IsValidBuffer())
	{
		return false;
	}

	EnqueueToGPUSync(
		[this, InFunction](FRHICommandListImmediate& RHICmdList)
		{
			FBufferRHIRef UploadBuffer = GetUploadBuffer(RHICmdList);
			void* Data = RHICmdList.LockBuffer(UploadBuffer, 0, UploadBuffer->GetSize(), EResourceLockMode::RLM_WriteOnly);
			if (Data)
			{
				InFunction(Data);
				RHICmdList.UnlockBuffer(UploadBuffer);
			}
			RHICmdList.Transition(FRHITransitionInfo(UploadBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopyDest));
			RHICmdList.CopyBufferRegion(BufferRHIRef, 0, UploadBuffer, 0, UploadBuffer->GetSize());
		});

	return true;
}

void UCompushadyResource::MapReadAndExecuteInGameThread(TFunction<void(const void*)> InFunction, const FCompushadySignaled& OnSignaled)
{
	auto Wrapper = [this, InFunction](const void* Data)
		{
			FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, InFunction, Data]()
				{
					InFunction(Data);
				}, TStatId(), nullptr, ENamedThreads::GameThread);
			FTaskGraphInterface::Get().WaitUntilTaskCompletes(Task);
		};

	MapReadAndExecute(Wrapper, OnSignaled);
}

void UCompushadyResource::MapWriteAndExecuteInGameThread(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled)
{
	auto Wrapper = [this, InFunction](void* Data)
		{
			FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, InFunction, Data]()
				{
					InFunction(Data);
				}, TStatId(), nullptr, ENamedThreads::GameThread);
			FTaskGraphInterface::Get().WaitUntilTaskCompletes(Task);
		};

	MapWriteAndExecute(Wrapper, OnSignaled);
}

bool UCompushadyResource::MapTextureSliceAndExecuteSync(TFunction<void(const void*, const int32)> InFunction, const int32 Slice)
{
	if (IsRunning() || !IsValidTexture())
	{
		return false;
	}

	FRHICopyTextureInfo CopyTextureInfo;
	CopyTextureInfo.Size.X = TextureRHIRef->GetSizeX();
	CopyTextureInfo.Size.Y = TextureRHIRef->GetSizeY();
	CopyTextureInfo.Size.Z = 1;

	const ETextureDimension Dimension = TextureRHIRef->GetDesc().Dimension;
	if (Dimension == ETextureDimension::Texture2DArray)
	{
		CopyTextureInfo.SourceSliceIndex = Slice;
	}
	else if (Dimension == ETextureDimension::Texture3D)
	{
		CopyTextureInfo.SourcePosition.Z = Slice;
	}

	EnqueueToGPUSync(
		[this, InFunction, &CopyTextureInfo](FRHICommandListImmediate& RHICmdList)
		{
			FTextureRHIRef ReadbackTexture = GetReadbackTexture();
			RHICmdList.Transition(FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.CopyTexture(TextureRHIRef, ReadbackTexture, CopyTextureInfo);
			WaitForGPU(RHICmdList);
			int32 Width = 0;
			int32 Height = 0;
			void* Data = nullptr;
			RHICmdList.MapStagingSurface(ReadbackTexture, Data, Width, Height);
			if (Data)
			{
				InFunction(Data, Width * GPixelFormats[TextureRHIRef->GetFormat()].BlockBytes);
				RHICmdList.UnmapStagingSurface(ReadbackTexture);
			}
		});

	return true;
}

namespace Compushady
{
	namespace Pipeline
	{
		template<typename SHADER_TYPE>
		void SetupParameters(FRHICommandList& RHICmdList, SHADER_TYPE Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FPostProcessMaterialInputs& PPInputs)
		{
#if COMPUSHADY_UE_VERSION >= 53
			FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
#endif

			for (int32 Index = 0; Index < ResourceArray.CBVs.Num(); Index++)
			{
				if (ResourceArray.CBVs[Index]->BufferDataIsDirty())
				{
					ResourceArray.CBVs[Index]->SyncBufferData(RHICmdList);
				}
#if COMPUSHADY_UE_VERSION >= 53
				BatchedParameters.SetShaderUniformBuffer(ResourceBindings.CBVs[Index].SlotIndex, ResourceArray.CBVs[Index]->GetRHI());
#else
				RHICmdList.SetShaderUniformBuffer(Shader, ResourceBindings.CBVs[Index].SlotIndex, ResourceArray.CBVs[Index]->GetRHI());
#endif
			}

			for (int32 Index = 0; Index < ResourceArray.SRVs.Num(); Index++)
			{
				if (!ResourceArray.SRVs[Index]->IsSceneTexture())
				{
					RHICmdList.Transition(ResourceArray.SRVs[Index]->GetRHITransitionInfo());
#if COMPUSHADY_UE_VERSION >= 53
					BatchedParameters.SetShaderResourceViewParameter(ResourceBindings.SRVs[Index].SlotIndex, ResourceArray.SRVs[Index]->GetRHI());
#else
					RHICmdList.SetShaderResourceViewParameter(Shader, ResourceBindings.SRVs[Index].SlotIndex, ResourceArray.SRVs[Index]->GetRHI());
#endif
				}
				else
				{
					FTextureRHIRef Texture = ResourceArray.SRVs[Index]->GetRHI(PPInputs);
					RHICmdList.Transition(FRHITransitionInfo(Texture, ERHIAccess::RTV, ERHIAccess::SRVMask));
#if COMPUSHADY_UE_VERSION >= 53
					BatchedParameters.SetShaderTexture(ResourceBindings.SRVs[Index].SlotIndex, Texture);
#else
					RHICmdList.SetShaderTexture(Shader, ResourceBindings.SRVs[Index].SlotIndex, Texture);
#endif
				}
				}

			for (int32 Index = 0; Index < ResourceArray.UAVs.Num(); Index++)
			{
				RHICmdList.Transition(ResourceArray.UAVs[Index]->GetRHITransitionInfo());
#if COMPUSHADY_UE_VERSION >= 53
				BatchedParameters.SetUAVParameter(ResourceBindings.UAVs[Index].SlotIndex, ResourceArray.UAVs[Index]->GetRHI());
#else
				RHICmdList.SetUAVParameter(Shader, ResourceBindings.UAVs[Index].SlotIndex, ResourceArray.UAVs[Index]->GetRHI());
#endif
			}

			for (int32 Index = 0; Index < ResourceArray.Samplers.Num(); Index++)
			{
#if COMPUSHADY_UE_VERSION >= 53
				BatchedParameters.SetShaderSampler(ResourceBindings.Samplers[Index].SlotIndex, ResourceArray.Samplers[Index]->GetRHI());
#else
				RHICmdList.SetShaderSampler(Shader, ResourceBindings.Samplers[Index].SlotIndex, ResourceArray.Samplers[Index]->GetRHI());
#endif
			}

#if COMPUSHADY_UE_VERSION >= 53
			RHICmdList.SetBatchedShaderParameters(Shader, BatchedParameters);
#endif
			}

		// Special case for UE 5.2 where a VertexShader and a MeshShader cannot have UAVs
#if COMPUSHADY_UE_VERSION < 53
		template<>
		void SetupParameters(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FPostProcessMaterialInputs& PPInputs)
		{
			for (int32 Index = 0; Index < ResourceArray.CBVs.Num(); Index++)
			{
				if (ResourceArray.CBVs[Index]->BufferDataIsDirty())
				{
					ResourceArray.CBVs[Index]->SyncBufferData(RHICmdList);
				}
				RHICmdList.SetShaderUniformBuffer(Shader, ResourceBindings.CBVs[Index].SlotIndex, ResourceArray.CBVs[Index]->GetRHI());
			}

			for (int32 Index = 0; Index < ResourceArray.SRVs.Num(); Index++)
			{
				RHICmdList.Transition(ResourceArray.SRVs[Index]->GetRHITransitionInfo());
				RHICmdList.SetShaderResourceViewParameter(Shader, ResourceBindings.SRVs[Index].SlotIndex, ResourceArray.SRVs[Index]->GetRHI());
			}

			for (int32 Index = 0; Index < ResourceArray.Samplers.Num(); Index++)
			{
				RHICmdList.SetShaderSampler(Shader, ResourceBindings.Samplers[Index].SlotIndex, ResourceArray.Samplers[Index]->GetRHI());
			}
		}

		template<>
		void SetupParameters(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FPostProcessMaterialInputs& PPInputs)
		{
			for (int32 Index = 0; Index < ResourceArray.CBVs.Num(); Index++)
			{
				if (ResourceArray.CBVs[Index]->BufferDataIsDirty())
				{
					ResourceArray.CBVs[Index]->SyncBufferData(RHICmdList);
				}
				RHICmdList.SetShaderUniformBuffer(Shader, ResourceBindings.CBVs[Index].SlotIndex, ResourceArray.CBVs[Index]->GetRHI());
			}

			for (int32 Index = 0; Index < ResourceArray.SRVs.Num(); Index++)
			{
				RHICmdList.Transition(ResourceArray.SRVs[Index]->GetRHITransitionInfo());
				RHICmdList.SetShaderResourceViewParameter(Shader, ResourceBindings.SRVs[Index].SlotIndex, ResourceArray.SRVs[Index]->GetRHI());
			}

			for (int32 Index = 0; Index < ResourceArray.Samplers.Num(); Index++)
			{
				RHICmdList.SetShaderSampler(Shader, ResourceBindings.Samplers[Index].SlotIndex, ResourceArray.Samplers[Index]->GetRHI());
			}
		}
#endif
	}
			}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, {});
}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, {});
}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, {});
}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FPixelShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FPostProcessMaterialInputs& PPInputs)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, PPInputs);
}
void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FRayTracingShaderBindingsWriter& ShaderBindingsWriter, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings)
{
	FRayTracingShaderBindingsWriter GlobalResources;

	for (int32 Index = 0; Index < ResourceArray.CBVs.Num(); Index++)
	{
		if (ResourceArray.CBVs[Index]->BufferDataIsDirty())
		{
			ResourceArray.CBVs[Index]->SyncBufferData(RHICmdList);
		}

		GlobalResources.SetUniformBuffer(ResourceBindings.CBVs[Index].SlotIndex, ResourceArray.CBVs[Index]->GetRHI());
	}

	for (int32 Index = 0; Index < ResourceArray.SRVs.Num(); Index++)
	{
		RHICmdList.Transition(ResourceArray.SRVs[Index]->GetRHITransitionInfo());
		GlobalResources.SetSRV(ResourceBindings.SRVs[Index].SlotIndex, ResourceArray.SRVs[Index]->GetRHI());
	}

	for (int32 Index = 0; Index < ResourceArray.UAVs.Num(); Index++)
	{
		RHICmdList.Transition(ResourceArray.UAVs[Index]->GetRHITransitionInfo());
		GlobalResources.SetUAV(ResourceBindings.UAVs[Index].SlotIndex, ResourceArray.UAVs[Index]->GetRHI());
	}
}

void ICompushadyPipeline::TrackResource(UObject* InResource)
{
	CurrentTrackedResources.Add(TStrongObjectPtr<UObject>(InResource));
}

void ICompushadyPipeline::TrackResources(const FCompushadyResourceArray& ResourceArray)
{
	for (UObject* Resource : ResourceArray.CBVs)
	{
		TrackResource(Resource);
	}

	for (UObject* Resource : ResourceArray.SRVs)
	{
		TrackResource(Resource);
	}

	for (UObject* Resource : ResourceArray.UAVs)
	{
		TrackResource(Resource);
	}
}

void ICompushadyPipeline::UntrackResources()
{
	CurrentTrackedResources.Empty();
}

void ICompushadyPipeline::OnSignalReceived()
{
	UntrackResources();
}

bool Compushady::Utils::ValidateResourceBindings(const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	const TArray<UCompushadyCBV*>& CBVs = ResourceArray.CBVs;
	const TArray<UCompushadySRV*>& SRVs = ResourceArray.SRVs;
	const TArray<UCompushadyUAV*>& UAVs = ResourceArray.UAVs;
	const TArray<UCompushadySampler*>& Samplers = ResourceArray.Samplers;

	if (CBVs.Num() != ResourceBindings.CBVs.Num())
	{
		ErrorMessages = FString::Printf(TEXT("Expected %d VS CBVs got %d"), ResourceBindings.CBVs.Num(), CBVs.Num());
		return false;
	}

	for (int32 Index = 0; Index < CBVs.Num(); Index++)
	{
		if (!CBVs[Index])
		{
			ErrorMessages = FString::Printf(TEXT("CBV %d cannot be null"), Index);
			return false;
		}
	}

	if (SRVs.Num() != ResourceBindings.SRVs.Num())
	{
		ErrorMessages = FString::Printf(TEXT("Expected %d SRVs got %d"), ResourceBindings.SRVs.Num(), SRVs.Num());
		return false;
	}

	for (int32 Index = 0; Index < SRVs.Num(); Index++)
	{
		if (!SRVs[Index])
		{
			ErrorMessages = FString::Printf(TEXT("SRV %d (%s) cannot be null"), Index, *(ResourceBindings.SRVs[Index].Name));
			return false;
		}
	}

	if (UAVs.Num() != ResourceBindings.UAVs.Num())
	{
		ErrorMessages = FString::Printf(TEXT("Expected %d UAVs got %d"), ResourceBindings.UAVs.Num(), UAVs.Num());
		return false;
	}

	for (int32 Index = 0; Index < UAVs.Num(); Index++)
	{
		if (!UAVs[Index])
		{
			ErrorMessages = FString::Printf(TEXT("UAV %d cannot be null"), Index);
			return false;
		}
	}

	if (Samplers.Num() != ResourceBindings.Samplers.Num())
	{
		ErrorMessages = FString::Printf(TEXT("Expected %d Samplers got %d"), ResourceBindings.Samplers.Num(), Samplers.Num());
		return false;
	}

	for (int32 Index = 0; Index < Samplers.Num(); Index++)
	{
		if (!Samplers[Index])
		{
			ErrorMessages = FString::Printf(TEXT("Sampler %d cannot be null"), Index);
			return false;
		}
	}

	return true;
}

bool ICompushadyPipeline::CheckResourceBindings(const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FCompushadySignaled& OnSignaled)
{
	const TArray<UCompushadyCBV*>& CBVs = ResourceArray.CBVs;
	const TArray<UCompushadySRV*>& SRVs = ResourceArray.SRVs;
	const TArray<UCompushadyUAV*>& UAVs = ResourceArray.UAVs;
	const TArray<UCompushadySampler*>& Samplers = ResourceArray.Samplers;

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(ResourceArray, ResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return false;
	}

	return true;
}

bool Compushady::Utils::CreateResourceBindings(Compushady::FCompushadyShaderResourceBindings InBindings, FCompushadyResourceBindings& OutBindings, FString& ErrorMessages)
{
	// configure CBV resource bindings
	for (int32 Index = 0; Index < InBindings.CBVs.Num(); Index++)
	{
		const Compushady::FCompushadyShaderResourceBinding& ShaderResourceBinding = InBindings.CBVs[Index];
		if (ShaderResourceBinding.SlotIndex + 1 > OutBindings.NumCBVs)
		{
			OutBindings.NumCBVs = ShaderResourceBinding.SlotIndex + 1;
		}

		FCompushadyResourceBinding ResourceBinding;
		ResourceBinding.BindingIndex = ShaderResourceBinding.BindingIndex;
		ResourceBinding.SlotIndex = ShaderResourceBinding.SlotIndex;
		ResourceBinding.Name = ShaderResourceBinding.Name;

		OutBindings.CBVs.Add(ResourceBinding);
		OutBindings.CBVsMap.Add(ResourceBinding.Name, ResourceBinding);
		OutBindings.CBVsSlotMap.Add(ResourceBinding.SlotIndex, ResourceBinding);
	}

	// check for holes in the CBVs
	for (int32 Index = 0; Index < static_cast<int32>(OutBindings.NumCBVs); Index++)
	{
		bool bFound = false;
		for (const FCompushadyResourceBinding& Binding : OutBindings.CBVs)
		{
			if (Binding.SlotIndex == Index)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			ErrorMessages = "Binding holes not allowed in CBVs";
			return false;
		}
	}

	// configure SRV resource bindings
	for (int32 Index = 0; Index < InBindings.SRVs.Num(); Index++)
	{
		Compushady::FCompushadyShaderResourceBinding& ShaderResourceBinding = InBindings.SRVs[Index];
		if (ShaderResourceBinding.SlotIndex + 1 > OutBindings.NumSRVs)
		{
			OutBindings.NumSRVs = ShaderResourceBinding.SlotIndex + 1;
		}

		FCompushadyResourceBinding ResourceBinding;
		ResourceBinding.BindingIndex = ShaderResourceBinding.BindingIndex;
		ResourceBinding.SlotIndex = ShaderResourceBinding.SlotIndex;
		ResourceBinding.Name = ShaderResourceBinding.Name;
		OutBindings.SRVs.Add(ResourceBinding);
		OutBindings.SRVsMap.Add(ResourceBinding.Name, ResourceBinding);
		OutBindings.SRVsSlotMap.Add(ResourceBinding.SlotIndex, ResourceBinding);
	}

	// configure UAV resource bindings
	for (int32 Index = 0; Index < InBindings.UAVs.Num(); Index++)
	{
		Compushady::FCompushadyShaderResourceBinding& ShaderResourceBinding = InBindings.UAVs[Index];
		if (ShaderResourceBinding.SlotIndex + 1 > OutBindings.NumUAVs)
		{
			OutBindings.NumUAVs = ShaderResourceBinding.SlotIndex + 1;
		}

		FCompushadyResourceBinding ResourceBinding;
		ResourceBinding.BindingIndex = ShaderResourceBinding.BindingIndex;
		ResourceBinding.SlotIndex = ShaderResourceBinding.SlotIndex;
		ResourceBinding.Name = ShaderResourceBinding.Name;
		OutBindings.UAVs.Add(ResourceBinding);
		OutBindings.UAVsMap.Add(ResourceBinding.Name, ResourceBinding);
		OutBindings.UAVsSlotMap.Add(ResourceBinding.SlotIndex, ResourceBinding);
	}

	// configure Samplers resource bindings
	for (int32 Index = 0; Index < InBindings.Samplers.Num(); Index++)
	{
		Compushady::FCompushadyShaderResourceBinding& ShaderResourceBinding = InBindings.Samplers[Index];
		if (ShaderResourceBinding.SlotIndex + 1 > OutBindings.NumSamplers)
		{
			OutBindings.NumSamplers = ShaderResourceBinding.SlotIndex + 1;
		}

		FCompushadyResourceBinding ResourceBinding;
		ResourceBinding.BindingIndex = ShaderResourceBinding.BindingIndex;
		ResourceBinding.SlotIndex = ShaderResourceBinding.SlotIndex;
		ResourceBinding.Name = ShaderResourceBinding.Name;
		OutBindings.Samplers.Add(ResourceBinding);
		OutBindings.SamplersMap.Add(ResourceBinding.Name, ResourceBinding);
		OutBindings.SamplersSlotMap.Add(ResourceBinding.SlotIndex, ResourceBinding);
	}

	return true;
}

FPixelShaderRHIRef Compushady::Utils::CreatePixelShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	FIntVector ThreadGroupSize;
	TArray<uint8> PixelShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings;
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, "ps_6_0", PixelShaderByteCode, PixelShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return nullptr;
	}

	if (!Compushady::Utils::CreateResourceBindings(PixelShaderResourceBindings, ResourceBindings, ErrorMessages))
	{
		return nullptr;
	}

	TArray<uint8> PSByteCode;
	FSHAHash PSHash;
	if (!Compushady::ToUnrealShader(PixelShaderByteCode, PSByteCode, PixelShaderResourceBindings.CBVs.Num(), PixelShaderResourceBindings.SRVs.Num(), PixelShaderResourceBindings.UAVs.Num(), PixelShaderResourceBindings.Samplers.Num(), PSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the pixel shader";
		return nullptr;
	}

	FPixelShaderRHIRef PixelShaderRef = RHICreatePixelShader(PSByteCode, PSHash);
	if (!PixelShaderRef.IsValid() || !PixelShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Pixel Shader";
		return nullptr;
	}

	PixelShaderRef->SetHash(PSHash);

	return PixelShaderRef;
}