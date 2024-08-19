// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadyTypes.h"
#include "CompushadyCBV.h"
#include "CompushadySampler.h"
#include "CompushadySRV.h"
#include "CompushadyUAV.h"
#include "CommonRenderResources.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Engine/Canvas.h"
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
		UploadBufferRHIRef = COMPUSHADY_CREATE_BUFFER(BufferRHIRef->GetSize(), EBufferUsageFlags::Dynamic, BufferRHIRef->GetStride(), ERHIAccess::CopySrc, ResourceCreateInfo);
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

bool UCompushadyResource::UpdateTextureSliceSyncWithFunction(const uint8* Ptr, const int64 Size, const int32 Slice, TFunction<void(FRHICommandListImmediate& RHICmdList, void* Data, const uint32 SourcePitch, const uint32 DestPitch)> InFunction)
{
	if (IsRunning() || !IsValidTexture())
	{
		return false;
	}

	const uint32 RowPitch = TextureRHIRef->GetSizeX() * GPixelFormats[TextureRHIRef->GetFormat()].BlockBytes;

	ENQUEUE_RENDER_COMMAND(DoCompushadyUpdateTexture)(
		[this, Ptr, Size, Slice, RowPitch, InFunction](FRHICommandListImmediate& RHICmdList)
		{
			const ETextureDimension Dimension = TextureRHIRef->GetDesc().Dimension;
			if (Dimension == ETextureDimension::Texture2D)
			{
				uint32 DestStride;
				void* Data = RHICmdList.LockTexture2D(TextureRHIRef, 0, EResourceLockMode::RLM_WriteOnly, DestStride, false);
				if (Data)
				{
					InFunction(RHICmdList, Data, RowPitch, DestStride);
					RHICmdList.UnlockTexture2D(TextureRHIRef, 0, false);
				}
			}
			else if (Dimension == ETextureDimension::Texture2DArray)
			{
				uint32 DestStride;
				void* Data = RHICmdList.LockTexture2DArray(TextureRHIRef, Slice, 0, EResourceLockMode::RLM_WriteOnly, DestStride, false);
				if (Data)
				{
					InFunction(RHICmdList, Data, RowPitch, DestStride);
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

bool UCompushadyResource::UpdateTextureSliceSync(const uint8* Ptr, const int64 Size, const int32 Slice)
{
	return UpdateTextureSliceSyncWithFunction(Ptr, Size, Slice, [&](FRHICommandListImmediate& RHICmdList, void* Data, const uint32 SourcePitch, const uint32 DestPitch)
		{
			const ETextureDimension Dimension = TextureRHIRef->GetDesc().Dimension;
			if (Dimension == ETextureDimension::Texture2D)
			{
				CopyTextureData2D(Ptr, Data, TextureRHIRef->GetSizeY(), TextureRHIRef->GetFormat(), SourcePitch, DestPitch);
			}
			else if (Dimension == ETextureDimension::Texture2DArray)
			{
				CopyTextureData2D(Ptr, Data, TextureRHIRef->GetSizeY(), TextureRHIRef->GetFormat(), SourcePitch, DestPitch);
			}
			else if (Dimension == ETextureDimension::Texture3D)
			{
				// no need to copy here, as we are using the RHICmdList.UpdateTexture3D
			}
		});
}

bool UCompushadyResource::UpdateTextureSliceSync(const TArray<uint8>& Pixels, const int32 Slice)
{
	return UpdateTextureSliceSync(Pixels.GetData(), Pixels.Num(), Slice);
}

bool UCompushadyResource::ClearBufferWithByteSync(const uint8 Value)
{
	if (!IsValidBuffer())
	{
		return false;
	}

	const int64 BufferSize = GetBufferSize();
	return MapWriteAndExecuteSync([Value, BufferSize](void* Data)
		{
			FMemory::Memset(Data, Value, BufferSize);
			return true;
		});
}

bool UCompushadyResource::ClearBufferWithFloatSync(const float Value)
{
	if (!IsValidBuffer())
	{
		return false;
	}

	const int64 BufferSize = GetBufferSize() / sizeof(float);
	return MapWriteAndExecuteSync([Value, BufferSize](void* Data)
		{
			float* Ptr = reinterpret_cast<float*>(Data);
			for (int64 Index = 0; Index < BufferSize; Index++)
			{
				Ptr[Index] = Value;
			}
			return true;
		});
}

bool UCompushadyResource::ClearBufferWithIntSync(const int32 Value)
{
	if (!IsValidBuffer())
	{
		return false;
	}

	const int64 BufferSize = GetBufferSize() / sizeof(int32);
	return MapWriteAndExecuteSync([Value, BufferSize](void* Data)
		{
			int32* Ptr = reinterpret_cast<int32*>(Data);
			for (int64 Index = 0; Index < BufferSize; Index++)
			{
				Ptr[Index] = Value;
			}
			return true;
		});
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

bool UCompushadyResource::ReadbackTextureToPNGFileSync(const FString& Filename, FString& ErrorMessages)
{
	if (!IsValidTexture())
	{
		ErrorMessages = "The resource is not a valid Texture";
		return false;
	}

	if (GetTextureRHI()->GetDesc().Format != EPixelFormat::PF_B8G8R8A8 && GetTextureRHI()->GetDesc().Format != EPixelFormat::PF_R8G8B8A8)
	{
		ErrorMessages = "Only RGBA8 and BGRA8 textures are supported";
		return false;
	}

	auto PngWriter = [this, Filename](const void* Data, const uint32 Stride)
		{
			const FIntVector Size = GetTextureSize();
			IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

			TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
			if (ImageWrapper.IsValid())
			{
				if (ImageWrapper->SetRaw(Data, Size.X * Size.Y * GPixelFormats[GetTextureRHI()->GetDesc().Format].BlockBytes,
					Size.X, Size.Y, GetTextureRHI()->GetDesc().Format == EPixelFormat::PF_B8G8R8A8 ? ERGBFormat::BGRA : ERGBFormat::RGBA, 8))
				{

					TArray64<uint8> CompressedBytes = ImageWrapper->GetCompressed();
					FFileHelper::SaveArrayToFile(CompressedBytes, *Filename);
				}
			}
		};

	return MapTextureSliceAndExecuteSync(PngWriter, 0);
}

bool UCompushadyResource::ReadbackTextureToTIFFFileSync(const FString& Filename, const FString& ImageDescription, FString& ErrorMessages)
{
	if (!IsValidTexture())
	{
		ErrorMessages = "The resource is not a valid Texture";
		return false;
	}

	auto TIFFWriter = [this, Filename, ImageDescription](const void* Data, const uint32 Stride)
		{
			const FIntVector Size = GetTextureSize();
			const EPixelFormat PixelFormat = GetTextureRHI()->GetDesc().Format;
			TArray<uint8> TIFF;
			if (Compushady::Utils::GenerateTIFF(Data, Stride, Size.X, Size.Y, PixelFormat, ImageDescription, TIFF))
			{
				FFileHelper::SaveArrayToFile(TIFF, *Filename);
			}
		};

	return MapTextureSliceAndExecuteSync(TIFFWriter, 0);
}

bool UCompushadyResource::ReadbackBufferToWAVFileSync(const FString& Filename, const int64 Offset, const int64 Size, const int32 SampleRate, const int32 NumChannels, const EPixelFormat PixelFormat, const ECompushadyWAVFormat WAVFormat, FString& ErrorMessages)
{
	if (!IsValidBuffer())
	{
		ErrorMessages = "The resource is not a valid Buffer";
		return false;
	}

	int64 WantedSize = Size;
	if (WantedSize == 0)
	{
		WantedSize = GetBufferSize();
	}

	if (Offset < 0 || Offset + WantedSize > GetBufferSize())
	{
		ErrorMessages = "Invalid Buffer offset";
		return false;
	}

	if (!GPixelFormats[PixelFormat].Supported)
	{
		ErrorMessages = FString::Printf(TEXT("Unsupported Pixel Format %s"), GetPixelFormatString(PixelFormat));
		return false;
	}

	if (PixelFormat == EPixelFormat::PF_R32_FLOAT)
	{
		auto WAVWriter = [Filename, NumChannels, SampleRate, Offset, WantedSize](const void* Data)
			{
				TArray<uint8> WAV;
				const uint32 WantedSize32 = WantedSize;
				const uint32 OverallFileSize = 12 + 24 + 8 + WantedSize;
				WAV.Append({ 0x52, 0x49, 0x46, 0x46 }); // RIFF
				WAV.Append(reinterpret_cast<const uint8*>(&OverallFileSize), sizeof(uint32)); // file size - 8
				WAV.Append({ 0x57, 0x41, 0x56, 0x45 }); // WAVE

				WAV.Append({ 0x66, 0x6D, 0x74, 0x20 }); // fmt
				WAV.Append({ 0x10, 0, 0, 0 }); // chunk size (16)
				WAV.Append({ 3, 0 }); // Audio format (float)
				const uint16 NumChannels16 = NumChannels;
				const uint32 SampleRate32 = SampleRate;
				const uint16 BytesPerBlock = sizeof(float) * NumChannels;
				const uint32 BytesPerSec = SampleRate * BytesPerBlock;
				const uint16 BitsPerSample = 32;
				WAV.Append(reinterpret_cast<const uint8*>(&NumChannels16), sizeof(uint16)); // Number of channels
				WAV.Append(reinterpret_cast<const uint8*>(&SampleRate32), sizeof(uint32)); // Sample rate
				WAV.Append(reinterpret_cast<const uint8*>(&BytesPerSec), sizeof(uint32)); // Number of bytes to read per second
				WAV.Append(reinterpret_cast<const uint8*>(&BytesPerBlock), sizeof(uint16)); // Number of bytes per block
				WAV.Append(reinterpret_cast<const uint8*>(&BitsPerSample), sizeof(uint16)); // Number of bits per sample

				WAV.Append({ 0x64, 0x61, 0x74, 0x61 }); // data
				WAV.Append(reinterpret_cast<const uint8*>(&WantedSize32), sizeof(uint32)); // SampledData size

				WAV.Append(reinterpret_cast<const uint8*>(Data) + Offset, WantedSize);

				FFileHelper::SaveArrayToFile(WAV, *Filename);

				return true;
			};
		return MapReadAndExecuteSync(WAVWriter);
	}

	ErrorMessages = FString::Printf(TEXT("Unsupported Pixel Format %s"), GetPixelFormatString(PixelFormat));
	return false;
}

void UCompushadyResource::ReadbackBufferToFile(const FString& Filename, const int64 Offset, const int64 Size, const FCompushadySignaled& OnSignaled)
{
	auto FileWriter = [this, Filename, Size, Offset](const void* Data)
		{
			int32 RequiredSize = GetBufferSize();
			if (Size > 0)
			{
				RequiredSize = FMath::Min<int64>(Size, RequiredSize);
			}
			if (Offset + RequiredSize > GetBufferSize())
			{
				return;
			}
			if (RequiredSize > 0)
			{
				TArrayView<const uint8> ArrayView = TArrayView<const uint8>(reinterpret_cast<const uint8*>(Data) + Offset, RequiredSize);
				FFileHelper::SaveArrayToFile(ArrayView, *Filename);
			}
		};

	MapReadAndExecute(FileWriter, OnSignaled);
}

bool UCompushadyResource::ReadbackBufferToFileSync(const FString& Filename, const int64 Offset, const int64 Size, FString& ErrorMessages)
{
	auto FileWriter = [this, Filename, Size, Offset](const void* Data)
		{
			int32 RequiredSize = GetBufferSize();
			if (Size > 0)
			{
				RequiredSize = FMath::Min<int64>(Size, RequiredSize);
			}
			if (Offset + RequiredSize > GetBufferSize())
			{
				return false;
			}
			if (RequiredSize > 0)
			{
				TArrayView<const uint8> ArrayView = TArrayView<const uint8>(reinterpret_cast<const uint8*>(Data) + Offset, RequiredSize);
				return FFileHelper::SaveArrayToFile(ArrayView, *Filename);
			}
			return false;
		};

	return MapReadAndExecuteSync(FileWriter);
}

void UCompushadyResource::ReadbackBufferToFloatArray(const int32 Offset, const int32 Elements, const FCompushadySignaledWithFloatArrayPayload& OnSignaled)
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

bool UCompushadyResource::ReadbackBufferToFloatArraySync(const int64 Offset, const int64 Elements, TArray<float>& Floats, FString& ErrorMessages)
{
	if (IsRunning())
	{
		ErrorMessages = "The Resource is already being processed by another task";
		return false;
	}

	auto ToFloatArray = [this, Offset, Elements, &Floats, &ErrorMessages](const void* Data)
		{
			int64 NumElements = GetBufferSize() / sizeof(float);
			if (Elements > 0)
			{
				NumElements = FMath::Min<int64>(Elements, NumElements);
			}
			if ((Offset + static_cast<int64>(NumElements * sizeof(float))) > GetBufferSize())
			{
				ErrorMessages = "Invalid Offset and Size";
				return false;
			}
			Floats.AddUninitialized(NumElements);
			FMemory::Memcpy(Floats.GetData(), reinterpret_cast<const uint8*>(Data) + Offset, NumElements * sizeof(float));

			return true;
		};

	return MapReadAndExecuteSync(ToFloatArray);
}

bool UCompushadyResource::ReadbackBufferToByteArraySync(const int64 Offset, const int64 Size, TArray<uint8>& Bytes, FString& ErrorMessages)
{
	if (IsRunning())
	{
		ErrorMessages = "The Resource is already being processed by another task";
		return false;
	}

	auto ToFloatArray = [this, Offset, Size, &Bytes, &ErrorMessages](const void* Data)
		{
			int64 RequestedSize = GetBufferSize();
			if (Size > 0)
			{
				RequestedSize = FMath::Min<int64>(Size, RequestedSize);
			}
			if ((Offset + RequestedSize) > GetBufferSize())
			{
				ErrorMessages = "Invalid Offset and Size";
				return false;
			}
			Bytes.AddUninitialized(RequestedSize);
			FMemory::Memcpy(Bytes.GetData(), reinterpret_cast<const uint8*>(Data) + Offset, RequestedSize);

			return true;
		};

	return MapReadAndExecuteSync(ToFloatArray);
}

TArray<FVector> UCompushadyResource::ReadbackBufferFloatsToVectorArraySync(const int32 Offset, const int32 Elements, const int32 Stride)
{
	if (Offset >= GetBufferSize() || Stride <= 0 || Elements <= 0)
	{
		return {};
	}

	TArray<FVector> Vectors;
	Vectors.AddUninitialized(Elements);

	MapReadAndExecuteSync([this, Offset, Elements, Stride, &Vectors](const void* Data)
		{
			const int64 NumFloats = FMath::Min<int64>((GetBufferSize() - Offset) / Stride, Elements);
			const uint8* SourcePtr = reinterpret_cast<const uint8*>(Data) + Offset;

			for (int64 Index = 0; Index < NumFloats; Index++)
			{
				const float* Floats = reinterpret_cast<const float*>(SourcePtr);
				Vectors[Index].X = Floats[0];
				Vectors[Index].Y = Floats[1];
				Vectors[Index].Z = Floats[2];

				SourcePtr += Stride;
			}

			return true;
		});

	return Vectors;
}

TArray<FVector2D> UCompushadyResource::ReadbackBufferFloatsToVector2ArraySync(const int32 Offset, const int32 Elements, const int32 Stride)
{
	if (Offset >= GetBufferSize() || Stride <= 0 || Elements <= 0)
	{
		return {};
	}

	TArray<FVector2D> Vectors;
	Vectors.AddUninitialized(Elements);

	MapReadAndExecuteSync([this, Offset, Elements, Stride, &Vectors](const void* Data)
		{
			const int64 NumFloats = FMath::Min<int64>((GetBufferSize() - Offset) / Stride, Elements);
			const uint8* SourcePtr = reinterpret_cast<const uint8*>(Data) + Offset;

			for (int64 Index = 0; Index < NumFloats; Index++)
			{
				const float* Floats = reinterpret_cast<const float*>(SourcePtr);
				Vectors[Index].X = Floats[0];
				Vectors[Index].Y = Floats[1];

				SourcePtr += Stride;
			}

			return true;
		});

	return Vectors;
}

TArray<FVector4> UCompushadyResource::ReadbackBufferFloatsToVector4ArraySync(const int32 Offset, const int32 Elements, const int32 Stride)
{
	if (Offset >= GetBufferSize() || Stride <= 0 || Elements <= 0)
	{
		return {};
	}

	TArray<FVector4> Vectors;
	Vectors.AddUninitialized(Elements);

	MapReadAndExecuteSync([this, Offset, Elements, Stride, &Vectors](const void* Data)
		{
			const int64 NumFloats = FMath::Min<int64>((GetBufferSize() - Offset) / Stride, Elements);
			const uint8* SourcePtr = reinterpret_cast<const uint8*>(Data) + Offset;

			for (int64 Index = 0; Index < NumFloats; Index++)
			{
				const float* Floats = reinterpret_cast<const float*>(SourcePtr);
				Vectors[Index].X = Floats[0];
				Vectors[Index].Y = Floats[1];
				Vectors[Index].Z = Floats[2];
				Vectors[Index].W = Floats[3];

				SourcePtr += Stride;
			}
			return true;
		});

	return Vectors;
}

TArray<FLinearColor> UCompushadyResource::ReadbackBufferFloatsToLinearColorArraySync(const int32 Offset, const int32 Elements, const int32 Stride)
{
	if (Offset >= GetBufferSize() || Stride <= 0 || Elements <= 0)
	{
		return {};
	}

	TArray<FLinearColor> Vectors;
	Vectors.AddUninitialized(Elements);

	MapReadAndExecuteSync([this, Offset, Elements, Stride, &Vectors](const void* Data)
		{
			const int64 NumFloats = FMath::Min<int64>((GetBufferSize() - Offset) / Stride, Elements);
			const uint8* SourcePtr = reinterpret_cast<const uint8*>(Data) + Offset;

			for (int64 Index = 0; Index < NumFloats; Index++)
			{
				const float* Floats = reinterpret_cast<const float*>(SourcePtr);
				Vectors[Index].R = Floats[0];
				Vectors[Index].G = Floats[1];
				Vectors[Index].B = Floats[2];
				Vectors[Index].A = Floats[3];

				SourcePtr += Stride;
			}

			return true;
		});

	return Vectors;
}

TArray<int32> UCompushadyResource::ReadbackBufferIntsToIntArraySync(const int32 Offset, const int32 Elements, const int32 Stride)
{
	if (Offset >= GetBufferSize() || Stride <= 0 || Elements <= 0)
	{
		return {};
	}

	TArray<int32> Values;
	Values.AddUninitialized(Elements);

	MapReadAndExecuteSync([this, Offset, Elements, Stride, &Values](const void* Data)
		{
			const int64 NumFloats = FMath::Min<int64>((GetBufferSize() - Offset) / Stride, Elements);
			const uint8* SourcePtr = reinterpret_cast<const uint8*>(Data) + Offset;

			for (int64 Index = 0; Index < NumFloats; Index++)
			{
				const int32* Ints = reinterpret_cast<const int32*>(SourcePtr);
				Values[Index] = *Ints;
				SourcePtr += Stride;
			}

			return true;
		});

	return Values;
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
	if (IsValidTexture())
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

FIntVector UCompushadyResource::GetStructuredBufferThreadGroupSize(const FIntVector XYZ) const
{
	if (IsValidBuffer() && EnumHasAnyFlags(BufferRHIRef->GetUsage(), EBufferUsageFlags::StructuredBuffer))
	{
		if (XYZ.X <= 0 || XYZ.Y <= 0 || XYZ.Z <= 0)
		{
			return FIntVector(1, 1, 1);
		}

		uint32 X = BufferRHIRef->GetSize() / BufferRHIRef->GetStride();
		return FIntVector(
			FMath::DivideAndRoundUp(static_cast<int32>(X), XYZ.X),
			XYZ.Y,
			XYZ.Z
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

int32 UCompushadyResource::GetBufferStride() const
{
	if (BufferRHIRef.IsValid() && BufferRHIRef->IsValid())
	{
		return static_cast<int32>(BufferRHIRef->GetStride());
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

bool UCompushadyResource::MapReadAndExecuteSync(TFunction<bool(const void*)> InFunction)
{
	if (IsRunning())
	{
		return false;
	}

	if (!IsValidBuffer())
	{
		return false;
	}

	bool bSuccess = false;

	EnqueueToGPUSync(
		[this, InFunction, &bSuccess](FRHICommandListImmediate& RHICmdList)
		{
			FStagingBufferRHIRef StagingBuffer = GetStagingBuffer();
			RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.CopyToStagingBuffer(BufferRHIRef, StagingBuffer, 0, BufferRHIRef->GetSize());
			WaitForGPU(RHICmdList);
			void* Data = RHICmdList.LockStagingBuffer(StagingBuffer, nullptr, 0, BufferRHIRef->GetSize());
			if (Data)
			{
				bSuccess = InFunction(Data);
				RHICmdList.UnlockStagingBuffer(StagingBuffer);
			}
		});

	return bSuccess;
}

bool UCompushadyResource::MapWriteAndExecuteSync(TFunction<bool(void*)> InFunction)
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
		void SetupParametersRHI(FRHICommandList& RHICmdList, SHADER_TYPE Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV)
		{
#if COMPUSHADY_UE_VERSION >= 53
			FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
#endif

			for (int32 Index = 0; Index < ResourceBindings.CBVs.Num(); Index++)
			{
				FUniformBufferRHIRef BufferRHI = CBVFunction(Index);
				if (!BufferRHI)
				{
					continue;
				}
#if COMPUSHADY_UE_VERSION >= 53
				BatchedParameters.SetShaderUniformBuffer(ResourceBindings.CBVs[Index].SlotIndex, BufferRHI);
#else
				RHICmdList.SetShaderUniformBuffer(Shader, ResourceBindings.CBVs[Index].SlotIndex, BufferRHI);
#endif
			}

			for (int32 Index = 0; Index < ResourceBindings.SRVs.Num(); Index++)
			{
				TPair<FShaderResourceViewRHIRef, FTextureRHIRef> SRVPair = SRVFunction(Index);
				if (!SRVPair.Key && !SRVPair.Value)
				{
					continue;
				}

				if (SRVPair.Key)
				{
#if COMPUSHADY_UE_VERSION >= 53
					BatchedParameters.SetShaderResourceViewParameter(ResourceBindings.SRVs[Index].SlotIndex, SRVPair.Key);
#else
					RHICmdList.SetShaderResourceViewParameter(Shader, ResourceBindings.SRVs[Index].SlotIndex, SRVPair.Key);
#endif
				}
				else
				{
#if COMPUSHADY_UE_VERSION >= 53
					BatchedParameters.SetShaderTexture(ResourceBindings.SRVs[Index].SlotIndex, SRVPair.Value);
#else
					RHICmdList.SetShaderTexture(Shader, ResourceBindings.SRVs[Index].SlotIndex, SRVPair.Value);
#endif
				}
			}

			for (int32 Index = 0; Index < ResourceBindings.UAVs.Num(); Index++)
			{
				FUnorderedAccessViewRHIRef UAV = UAVFunction(Index);
				if (!UAV)
				{
					continue;
				}
#if COMPUSHADY_UE_VERSION >= 53
				BatchedParameters.SetUAVParameter(ResourceBindings.UAVs[Index].SlotIndex, UAV);
#else
				RHICmdList.SetUAVParameter(Shader, ResourceBindings.UAVs[Index].SlotIndex, UAV);
#endif
			}

			for (int32 Index = 0; Index < ResourceBindings.Samplers.Num(); Index++)
			{
				FSamplerStateRHIRef SamplerState = SamplerFunction(Index);
				if (!SamplerState)
				{
					continue;
				}
#if COMPUSHADY_UE_VERSION >= 53
				BatchedParameters.SetShaderSampler(ResourceBindings.Samplers[Index].SlotIndex, SamplerState);
#else
				RHICmdList.SetShaderSampler(Shader, ResourceBindings.Samplers[Index].SlotIndex, SamplerState);
#endif
			}

#if COMPUSHADY_UE_VERSION >= 53
			RHICmdList.SetBatchedShaderParameters(Shader, BatchedParameters);
#endif
		}

		// Special case for UE 5.2 where a VertexShader and a MeshShader cannot have UAVs
#if COMPUSHADY_UE_VERSION < 53
		template<>
		void SetupParametersRHI(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV)
		{
			for (int32 Index = 0; Index < ResourceBindings.CBVs.Num(); Index++)
			{
				FUniformBufferRHIRef BufferRHI = CBVFunction(Index);
				if (!BufferRHI)
				{
					continue;
				}
				RHICmdList.SetShaderUniformBuffer(Shader, ResourceBindings.CBVs[Index].SlotIndex, BufferRHI);
			}

			for (int32 Index = 0; Index < ResourceBindings.SRVs.Num(); Index++)
			{
				TPair<FShaderResourceViewRHIRef, FTextureRHIRef> SRVPair = SRVFunction(Index);
				if (!SRVPair.Key && !SRVPair.Value)
				{
					continue;
				}

				if (SRVPair.Key)
				{
					RHICmdList.SetShaderResourceViewParameter(Shader, ResourceBindings.SRVs[Index].SlotIndex, SRVPair.Key);
				}
				else
				{
					RHICmdList.SetShaderTexture(Shader, ResourceBindings.SRVs[Index].SlotIndex, SRVPair.Value);
				}
			}

			for (int32 Index = 0; Index < ResourceBindings.Samplers.Num(); Index++)
			{
				FSamplerStateRHIRef SamplerState = SamplerFunction(Index);
				if (!SamplerState)
				{
					continue;
				}
				RHICmdList.SetShaderSampler(Shader, ResourceBindings.Samplers[Index].SlotIndex, SamplerState);
			}
		}

		template<>
		void SetupParametersRHI(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV)
		{
			for (int32 Index = 0; Index < ResourceBindings.CBVs.Num(); Index++)
			{
				FUniformBufferRHIRef BufferRHI = CBVFunction(Index);
				if (!BufferRHI)
				{
					continue;
				}
				RHICmdList.SetShaderUniformBuffer(Shader, ResourceBindings.CBVs[Index].SlotIndex, BufferRHI);
			}

			for (int32 Index = 0; Index < ResourceBindings.SRVs.Num(); Index++)
			{
				TPair<FShaderResourceViewRHIRef, FTextureRHIRef> SRVPair = SRVFunction(Index);
				if (!SRVPair.Key && !SRVPair.Value)
				{
					continue;
				}

				if (SRVPair.Key)
				{
					RHICmdList.SetShaderResourceViewParameter(Shader, ResourceBindings.SRVs[Index].SlotIndex, SRVPair.Key);
				}
				else
				{
					RHICmdList.SetShaderTexture(Shader, ResourceBindings.SRVs[Index].SlotIndex, SRVPair.Value);
				}
			}

			for (int32 Index = 0; Index < ResourceBindings.Samplers.Num(); Index++)
			{
				FSamplerStateRHIRef SamplerState = SamplerFunction(Index);
				if (!SamplerState)
				{
					continue;
				}
				RHICmdList.SetShaderSampler(Shader, ResourceBindings.Samplers[Index].SlotIndex, SamplerState);
			}
		}
#endif

		template<typename SHADER_TYPE>
		void SetupParameters(FRHICommandList& RHICmdList, SHADER_TYPE Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FCompushadySceneTextures& SceneTextures, const bool bSyncCBV)
		{
			SetupParametersRHI(RHICmdList, Shader, ResourceBindings,
				[&](const int32 Index) // CBV
				{
					if (bSyncCBV && ResourceArray.CBVs[Index]->BufferDataIsDirty())
					{
						ResourceArray.CBVs[Index]->SyncBufferData(RHICmdList);
					}
					return ResourceArray.CBVs[Index]->GetRHI();
				},
				[&](const int32 Index) -> TPair<FShaderResourceViewRHIRef, FTextureRHIRef>// SRV
				{
					if (!ResourceArray.SRVs[Index]->IsSceneTexture())
					{
						RHICmdList.Transition(ResourceArray.SRVs[Index]->GetRHITransitionInfo());
						return { ResourceArray.SRVs[Index]->GetRHI() , nullptr };
					}
					else
					{
						TPair<FShaderResourceViewRHIRef, FTextureRHIRef> SRVOrTexture = ResourceArray.SRVs[Index]->GetRHI(SceneTextures);
						if (SRVOrTexture.Value)
						{
							RHICmdList.Transition(FRHITransitionInfo(SRVOrTexture.Value, ERHIAccess::Unknown, ERHIAccess::SRVMask));
						}
						else if (SRVOrTexture.Key)
						{
#if COMPUSHADY_UE_VERSION > 52
							FTextureRHIRef TextureToTransition = SRVOrTexture.Key->GetTexture();
							//RHICmdList.Transition(FRHITransitionInfo(TextureToTransition, ERHIAccess::Unknown, ERHIAccess::SRVMask));
#endif
						}
						return SRVOrTexture;
					}
				},
				[&](const int32 Index) // UAV
				{
					RHICmdList.Transition(ResourceArray.UAVs[Index]->GetRHITransitionInfo());
					return ResourceArray.UAVs[Index]->GetRHI();
				},
				[&](const int32 Index) // SamplerState
				{
					return ResourceArray.Samplers[Index]->GetRHI();
				}, bSyncCBV);
		}
	}
}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, {}, bSyncCBV);
}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FCompushadySceneTextures& SceneTextures, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, SceneTextures, bSyncCBV);
}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, {}, bSyncCBV);
}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, {}, bSyncCBV);
}

void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FPixelShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FCompushadySceneTextures& SceneTextures, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParameters(RHICmdList, Shader, ResourceArray, ResourceBindings, SceneTextures, bSyncCBV);
}
void Compushady::Utils::SetupPipelineParameters(FRHICommandList& RHICmdList, FRayTracingShaderBindingsWriter& ShaderBindingsWriter, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const bool bSyncCBV)
{
	FRayTracingShaderBindingsWriter GlobalResources;

	for (int32 Index = 0; Index < ResourceArray.CBVs.Num(); Index++)
	{
		if (bSyncCBV && ResourceArray.CBVs[Index]->BufferDataIsDirty())
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

void Compushady::Utils::SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParametersRHI(RHICmdList, Shader, ResourceBindings, CBVFunction, SRVFunction, UAVFunction, SamplerFunction, bSyncCBV);
}

void Compushady::Utils::SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParametersRHI(RHICmdList, Shader, ResourceBindings, CBVFunction, SRVFunction, UAVFunction, SamplerFunction, bSyncCBV);
}

void Compushady::Utils::SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParametersRHI(RHICmdList, Shader, ResourceBindings, CBVFunction, SRVFunction, UAVFunction, SamplerFunction, bSyncCBV);
}

void Compushady::Utils::SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FPixelShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV)
{
	Compushady::Pipeline::SetupParametersRHI(RHICmdList, Shader, ResourceBindings, CBVFunction, SRVFunction, UAVFunction, SamplerFunction, bSyncCBV);
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

void ICompushadyPipeline::TrackResourcesAndMarkAsRunning(const FCompushadyResourceArray& ResourceArray)
{
	TrackResources(ResourceArray);
	bRunning = true;
}

void ICompushadyPipeline::UntrackResourcesAndUnmarkAsRunning()
{
	UntrackResources();
	check(bRunning);
	bRunning = false;
}

void ICompushadyPipeline::OnSignalReceived()
{
	UntrackResources();
}

void UCompushadyResource::CopyToBuffer(UCompushadyResource* DestinationBuffer, const int64 Size, const int64 DestinationOffset, const int64 SourceOffset, const FCompushadySignaled& OnSignaled)
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
			// fast path, if at least one of the two resources is a UAV
			if (EnumHasAnyFlags(GetBufferRHI()->GetUsage(), EBufferUsageFlags::UnorderedAccess) || EnumHasAnyFlags(DestinationBuffer->GetBufferRHI()->GetUsage(), EBufferUsageFlags::UnorderedAccess))
			{
				RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
				RHICmdList.Transition(FRHITransitionInfo(DestinationBuffer->GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

				RHICmdList.CopyBufferRegion(DestinationBuffer->GetBufferRHI(), DestinationOffset, GetBufferRHI(), SourceOffset, RequiredSize);
			}
			else
			{
				// as unreal makes heavy reuse of resources, we need to rely on a temp UAV buffer
				FRHIResourceCreateInfo ResourceCreateInfo(TEXT(""));
				FBufferRHIRef TempBuffer = COMPUSHADY_CREATE_BUFFER(RequiredSize, EBufferUsageFlags::UnorderedAccess, DestinationBuffer->GetBufferRHI()->GetStride(), ERHIAccess::CopySrc, ResourceCreateInfo);
				RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
				RHICmdList.Transition(FRHITransitionInfo(TempBuffer, ERHIAccess::Unknown, ERHIAccess::CopyDest));

				RHICmdList.CopyBufferRegion(TempBuffer, 0, GetBufferRHI(), SourceOffset, RequiredSize);

				WaitForGPU(RHICmdList);

				RHICmdList.Transition(FRHITransitionInfo(TempBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc));
				RHICmdList.Transition(FRHITransitionInfo(DestinationBuffer->GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

				RHICmdList.CopyBufferRegion(DestinationBuffer->GetBufferRHI(), DestinationOffset, TempBuffer, 0, RequiredSize);
			}
		}, OnSignaled);
}

bool UCompushadyResource::CopyToBufferSync(UCompushadyResource* DestinationBuffer, const int64 Size, const int64 DestinationOffset, const int64 SourceOffset, FString& ErrorMessages)
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
			// fast path, if at least one of the two resources is a UAV
			if (EnumHasAnyFlags(GetBufferRHI()->GetUsage(), EBufferUsageFlags::UnorderedAccess) || EnumHasAnyFlags(DestinationBuffer->GetBufferRHI()->GetUsage(), EBufferUsageFlags::UnorderedAccess))
			{
				RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
				RHICmdList.Transition(FRHITransitionInfo(DestinationBuffer->GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

				RHICmdList.CopyBufferRegion(DestinationBuffer->GetBufferRHI(), DestinationOffset, GetBufferRHI(), SourceOffset, RequiredSize);
			}
			else
			{
				// as unreal makes heavy reuse of resources, we need to rely on a temp UAV buffer
				FRHIResourceCreateInfo ResourceCreateInfo(TEXT(""));
				FBufferRHIRef TempBuffer = COMPUSHADY_CREATE_BUFFER(RequiredSize, EBufferUsageFlags::UnorderedAccess, DestinationBuffer->GetBufferRHI()->GetStride(), ERHIAccess::CopySrc, ResourceCreateInfo);
				RHICmdList.Transition(FRHITransitionInfo(GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopySrc));
				RHICmdList.Transition(FRHITransitionInfo(TempBuffer, ERHIAccess::Unknown, ERHIAccess::CopyDest));

				RHICmdList.CopyBufferRegion(TempBuffer, 0, GetBufferRHI(), SourceOffset, RequiredSize);

				WaitForGPU(RHICmdList);

				RHICmdList.Transition(FRHITransitionInfo(TempBuffer, ERHIAccess::Unknown, ERHIAccess::CopySrc));
				RHICmdList.Transition(FRHITransitionInfo(DestinationBuffer->GetBufferRHI(), ERHIAccess::Unknown, ERHIAccess::CopyDest));

				RHICmdList.CopyBufferRegion(DestinationBuffer->GetBufferRHI(), DestinationOffset, TempBuffer, 0, RequiredSize);
			}
		});

	return true;
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

bool Compushady::Utils::ValidateResourceBindingsMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FCompushadyResourceBindings& ResourceBindings, FCompushadyResourceArray& ResourceArray, FString& ErrorMessages)
{
	for (int32 Index = 0; Index < ResourceBindings.CBVs.Num(); Index++)
	{
		const FString& Name = ResourceBindings.CBVs[Index].Name;
		if (!ResourceMap.Contains(Name))
		{
			ErrorMessages = FString::Printf(TEXT("Resource \"%s\" not found in supplied map"), *Name);
			return false;
		}
		UCompushadyCBV* CBV = Cast<UCompushadyCBV>(ResourceMap[Name].GetObject());
		if (!CBV)
		{
			ErrorMessages = FString::Printf(TEXT("Expected \"%s\" to be a CBV"), *Name);
			return false;
		}
		ResourceArray.CBVs.Add(CBV);
	}

	for (int32 Index = 0; Index < ResourceBindings.SRVs.Num(); Index++)
	{
		const FString& Name = ResourceBindings.SRVs[Index].Name;
		if (!ResourceMap.Contains(Name))
		{
			ErrorMessages = FString::Printf(TEXT("Resource \"%s\" not found in supplied map"), *Name);
			return false;
		}
		UCompushadySRV* SRV = Cast<UCompushadySRV>(ResourceMap[Name].GetObject());
		if (!SRV)
		{
			ErrorMessages = FString::Printf(TEXT("Expected \"%s\" to be a SRV"), *Name);
			return false;
		}
		ResourceArray.SRVs.Add(SRV);
	}

	for (int32 Index = 0; Index < ResourceBindings.UAVs.Num(); Index++)
	{
		const FString& Name = ResourceBindings.UAVs[Index].Name;
		if (!ResourceMap.Contains(Name))
		{
			ErrorMessages = FString::Printf(TEXT("Resource \"%s\" not found in supplied map"), *Name);
			return false;
		}
		UCompushadyUAV* UAV = Cast<UCompushadyUAV>(ResourceMap[Name].GetObject());
		if (!UAV)
		{
			ErrorMessages = FString::Printf(TEXT("Expected \"%s\" to be an UAV"), *Name);
			return false;
		}
		ResourceArray.UAVs.Add(UAV);
	}

	for (int32 Index = 0; Index < ResourceBindings.Samplers.Num(); Index++)
	{
		const FString& Name = ResourceBindings.Samplers[Index].Name;
		if (!ResourceMap.Contains(Name))
		{
			ErrorMessages = FString::Printf(TEXT("Sampler \"%s\" not found in supplied map"), *Name);
			return false;
		}
		UCompushadySampler* Sampler = Cast<UCompushadySampler>(ResourceMap[Name].GetObject());
		if (!Sampler)
		{
			ErrorMessages = FString::Printf(TEXT("Expected \"%s\" to be a Sampler"), *Name);
			return false;
		}
		ResourceArray.Samplers.Add(Sampler);
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

	OutBindings.InputSemantics = InBindings.InputSemantics;
	OutBindings.OutputSemantics = InBindings.OutputSemantics;

	return true;
}
FVertexShaderRHIRef Compushady::Utils::CreateVertexShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	const FString TargetProfile = "vs_6_0";

	FIntVector ThreadGroupSize;
	TArray<uint8> VertexShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings VertexShaderResourceBindings;
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, TargetProfile, VertexShaderByteCode, ErrorMessages, false))
	{
		return nullptr;
	}

	if (!Compushady::Utils::FinalizeShader(VertexShaderByteCode, TargetProfile, VertexShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, false))
	{
		return nullptr;
	}

	TArray<uint8> VSByteCode;
	FSHAHash VSHash;
	if (!Compushady::ToUnrealShader(VertexShaderByteCode, VSByteCode, VertexShaderResourceBindings.CBVs.Num(), VertexShaderResourceBindings.SRVs.Num(), VertexShaderResourceBindings.UAVs.Num(), VertexShaderResourceBindings.Samplers.Num(), VSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the vertex shader";
		return nullptr;
	}

	FVertexShaderRHIRef VertexShaderRef = RHICreateVertexShader(VSByteCode, VSHash);
	if (!VertexShaderRef.IsValid() || !VertexShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Vertex Shader";
		return nullptr;
	}

	VertexShaderRef->SetHash(VSHash);

	return VertexShaderRef;
}

FPixelShaderRHIRef Compushady::Utils::CreatePixelShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	const FString TargetProfile = "ps_6_0";

	FIntVector ThreadGroupSize;
	TArray<uint8> PixelShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings;
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, TargetProfile, PixelShaderByteCode, ErrorMessages, false))
	{
		return nullptr;
	}

	if (!Compushady::Utils::FinalizeShader(PixelShaderByteCode, TargetProfile, PixelShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, false))
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

FPixelShaderRHIRef Compushady::Utils::CreatePixelShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	const FString TargetProfile = "ps_6_0";

	FIntVector ThreadGroupSize;
	TArray<uint8> PixelShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings;
	if (!Compushady::CompileGLSL(ShaderCode, EntryPoint, TargetProfile, PixelShaderByteCode, ErrorMessages))
	{
		return nullptr;
	}

	if (!Compushady::Utils::FinalizeShader(PixelShaderByteCode, TargetProfile, PixelShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, true))
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

FComputeShaderRHIRef Compushady::Utils::CreateComputeShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages, const FString& TargetProfile)
{
	TArray<uint8> ComputeShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings ComputeShaderResourceBindings;
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, TargetProfile, ComputeShaderByteCode, ErrorMessages, false))
	{
		return nullptr;
	}

	if (!Compushady::Utils::FinalizeShader(ComputeShaderByteCode, TargetProfile, ComputeShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, false))
	{
		return nullptr;
	}

	TArray<uint8> CSByteCode;
	FSHAHash CSHash;
	if (!Compushady::ToUnrealShader(ComputeShaderByteCode, CSByteCode, ComputeShaderResourceBindings.CBVs.Num(), ComputeShaderResourceBindings.SRVs.Num(), ComputeShaderResourceBindings.UAVs.Num(), ComputeShaderResourceBindings.Samplers.Num(), CSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the compute shader";
		return nullptr;
	}

	FComputeShaderRHIRef ComputeShaderRef = RHICreateComputeShader(CSByteCode, CSHash);
	if (!ComputeShaderRef.IsValid() || !ComputeShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Compute Shader";
		return nullptr;
	}

	ComputeShaderRef->SetHash(CSHash);

	return ComputeShaderRef;
}

FComputeShaderRHIRef Compushady::Utils::CreateComputeShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	const FString TargetProfile = "cs_6_0";

	TArray<uint8> ComputeShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings ComputeShaderResourceBindings;
	if (!Compushady::CompileGLSL(ShaderCode, EntryPoint, TargetProfile, ComputeShaderByteCode, ErrorMessages))
	{
		return nullptr;
	}

	if (!Compushady::Utils::FinalizeShader(ComputeShaderByteCode, TargetProfile, ComputeShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, true))
	{
		return nullptr;
	}

	TArray<uint8> CSByteCode;
	FSHAHash CSHash;
	if (!Compushady::ToUnrealShader(ComputeShaderByteCode, CSByteCode, ComputeShaderResourceBindings.CBVs.Num(), ComputeShaderResourceBindings.SRVs.Num(), ComputeShaderResourceBindings.UAVs.Num(), ComputeShaderResourceBindings.Samplers.Num(), CSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the compute shader";
		return nullptr;
	}

	FComputeShaderRHIRef ComputeShaderRef = RHICreateComputeShader(CSByteCode, CSHash);
	if (!ComputeShaderRef.IsValid() || !ComputeShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Compute Shader";
		return nullptr;
	}

	ComputeShaderRef->SetHash(CSHash);

	return ComputeShaderRef;
}

FComputeShaderRHIRef Compushady::Utils::CreateComputeShaderFromSPIRVBlob(const TArray<uint8>& ShaderByteCode, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	const FString TargetProfile = "cs_6_0";

	TArray<uint8> ComputeShaderByteCode = ShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings ComputeShaderResourceBindings;

	if (!Compushady::Utils::FinalizeShader(ComputeShaderByteCode, TargetProfile, ComputeShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, true))
	{
		return nullptr;
	}

	TArray<uint8> CSByteCode;
	FSHAHash CSHash;
	if (!Compushady::ToUnrealShader(ComputeShaderByteCode, CSByteCode, ComputeShaderResourceBindings.CBVs.Num(), ComputeShaderResourceBindings.SRVs.Num(), ComputeShaderResourceBindings.UAVs.Num(), ComputeShaderResourceBindings.Samplers.Num(), CSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the compute shader";
		return nullptr;
	}

	FComputeShaderRHIRef ComputeShaderRef = RHICreateComputeShader(CSByteCode, CSHash);
	if (!ComputeShaderRef.IsValid() || !ComputeShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Compute Shader";
		return nullptr;
	}

	ComputeShaderRef->SetHash(CSHash);

	return ComputeShaderRef;
}

FMeshShaderRHIRef Compushady::Utils::CreateMeshShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	const FString TargetProfile = "ms_6_5";

	TArray<uint8> MeshShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings MeshShaderResourceBindings;
	if (!Compushady::CompileHLSL(ShaderCode, EntryPoint, TargetProfile, MeshShaderByteCode, ErrorMessages, false))
	{
		return nullptr;
	}

	if (!Compushady::Utils::FinalizeShader(MeshShaderByteCode, TargetProfile, MeshShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, false))
	{
		return nullptr;
	}

	TArray<uint8> MSByteCode;
	FSHAHash MSHash;
	if (!Compushady::ToUnrealShader(MeshShaderByteCode, MSByteCode, MeshShaderResourceBindings.CBVs.Num(), MeshShaderResourceBindings.SRVs.Num(), MeshShaderResourceBindings.UAVs.Num(), MeshShaderResourceBindings.Samplers.Num(), MSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the mesh shader";
		return nullptr;
	}

	FMeshShaderRHIRef MeshShaderRef = RHICreateMeshShader(MSByteCode, MSHash);
	if (!MeshShaderRef.IsValid() || !MeshShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Mesh Shader";
		return nullptr;
	}

	MeshShaderRef->SetHash(MSHash);

	return MeshShaderRef;
}

FMeshShaderRHIRef Compushady::Utils::CreateMeshShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	const FString TargetProfile = "ms_6_5";

	TArray<uint8> MeshShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings MeshShaderResourceBindings;
	if (!Compushady::CompileGLSL(ShaderCode, EntryPoint, TargetProfile, MeshShaderByteCode, ErrorMessages))
	{
		return nullptr;
	}

	if (!Compushady::Utils::FinalizeShader(MeshShaderByteCode, TargetProfile, MeshShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, true))
	{
		return nullptr;
	}

	TArray<uint8> MSByteCode;
	FSHAHash MSHash;
	if (!Compushady::ToUnrealShader(MeshShaderByteCode, MSByteCode, MeshShaderResourceBindings.CBVs.Num(), MeshShaderResourceBindings.SRVs.Num(), MeshShaderResourceBindings.UAVs.Num(), MeshShaderResourceBindings.Samplers.Num(), MSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the mesh shader";
		return nullptr;
	}

	FMeshShaderRHIRef MeshShaderRef = RHICreateMeshShader(MSByteCode, MSHash);
	if (!MeshShaderRef.IsValid() || !MeshShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Mesh Shader";
		return nullptr;
	}

	MeshShaderRef->SetHash(MSHash);

	return MeshShaderRef;
}

FVertexShaderRHIRef Compushady::Utils::CreateVertexShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	const FString TargetProfile = "vs_6_0";

	TArray<uint8> VertexShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings VertexShaderResourceBindings;
	if (!Compushady::CompileGLSL(ShaderCode, EntryPoint, TargetProfile, VertexShaderByteCode, ErrorMessages))
	{
		return nullptr;
	}

	FIntVector ThreadGroupSize;
	if (!Compushady::Utils::FinalizeShader(VertexShaderByteCode, TargetProfile, VertexShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, true))
	{
		return nullptr;
	}

	TArray<uint8> VSByteCode;
	FSHAHash VSHash;
	if (!Compushady::ToUnrealShader(VertexShaderByteCode, VSByteCode, VertexShaderResourceBindings.CBVs.Num(), VertexShaderResourceBindings.SRVs.Num(), VertexShaderResourceBindings.UAVs.Num(), VertexShaderResourceBindings.Samplers.Num(), VSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the vertex shader";
		return nullptr;
	}

	FVertexShaderRHIRef VertexShaderRef = RHICreateVertexShader(VSByteCode, VSHash);
	if (!VertexShaderRef.IsValid() || !VertexShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Vertex Shader";
		return nullptr;
	}

	VertexShaderRef->SetHash(VSHash);

	return VertexShaderRef;
}

FVertexShaderRHIRef Compushady::Utils::CreateVertexShaderFromHLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	TArray<uint8> ShaderCodeBytes;
	StringToShaderCode(ShaderCode, ShaderCodeBytes);
	return CreateVertexShaderFromHLSL(ShaderCodeBytes, EntryPoint, ResourceBindings, ErrorMessages);
}

FVertexShaderRHIRef Compushady::Utils::CreateVertexShaderFromGLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	TArray<uint8> ShaderCodeBytes;
	StringToShaderCode(ShaderCode, ShaderCodeBytes);
	return CreateVertexShaderFromGLSL(ShaderCodeBytes, EntryPoint, ResourceBindings, ErrorMessages);
}

FPixelShaderRHIRef Compushady::Utils::CreatePixelShaderFromHLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	TArray<uint8> ShaderCodeBytes;
	StringToShaderCode(ShaderCode, ShaderCodeBytes);
	return CreatePixelShaderFromHLSL(ShaderCodeBytes, EntryPoint, ResourceBindings, ErrorMessages);
}

FPixelShaderRHIRef Compushady::Utils::CreatePixelShaderFromGLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages)
{
	TArray<uint8> ShaderCodeBytes;
	StringToShaderCode(ShaderCode, ShaderCodeBytes);
	return CreatePixelShaderFromGLSL(ShaderCodeBytes, EntryPoint, ResourceBindings, ErrorMessages);
}

FComputeShaderRHIRef Compushady::Utils::CreateComputeShaderFromHLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	TArray<uint8> ShaderCodeBytes;
	StringToShaderCode(ShaderCode, ShaderCodeBytes);
	return CreateComputeShaderFromHLSL(ShaderCodeBytes, EntryPoint, ResourceBindings, ThreadGroupSize, ErrorMessages);
}

FComputeShaderRHIRef Compushady::Utils::CreateComputeShaderFromGLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	TArray<uint8> ShaderCodeBytes;
	StringToShaderCode(ShaderCode, ShaderCodeBytes);
	return CreateComputeShaderFromGLSL(ShaderCodeBytes, EntryPoint, ResourceBindings, ThreadGroupSize, ErrorMessages);
}

FMeshShaderRHIRef Compushady::Utils::CreateMeshShaderFromHLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	TArray<uint8> ShaderCodeBytes;
	StringToShaderCode(ShaderCode, ShaderCodeBytes);
	return CreateMeshShaderFromHLSL(ShaderCodeBytes, EntryPoint, ResourceBindings, ThreadGroupSize, ErrorMessages);
}

FMeshShaderRHIRef Compushady::Utils::CreateMeshShaderFromGLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	TArray<uint8> ShaderCodeBytes;
	StringToShaderCode(ShaderCode, ShaderCodeBytes);
	return CreateMeshShaderFromGLSL(ShaderCodeBytes, EntryPoint, ResourceBindings, ThreadGroupSize, ErrorMessages);
}

void Compushady::Utils::RasterizeSimplePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FVertexShaderRHIRef VertexShaderRef, FPixelShaderRHIRef PixelShaderRef, FTextureRHIRef RenderTarget, TFunction<void()> InFunction, const FCompushadyRasterizerConfig& RasterizerConfig)
{
	TArray<FTextureRHIRef> RenderTargets;
	if (RenderTarget)
	{
		RenderTargets.Add(RenderTarget);
	}
	RasterizeSimplePass_RenderThread(PassName, RHICmdList, VertexShaderRef, PixelShaderRef, RenderTargets, nullptr, InFunction, RasterizerConfig);
}

void Compushady::Utils::RasterizeSimplePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FVertexShaderRHIRef VertexShaderRef, FPixelShaderRHIRef PixelShaderRef, FTextureRHIRef RenderTarget, FTextureRHIRef DepthStencil, TFunction<void()> InFunction, const FCompushadyRasterizerConfig& RasterizerConfig)
{
	TArray<FTextureRHIRef> RenderTargets;
	if (RenderTarget)
	{
		RenderTargets.Add(RenderTarget);
	}
	RasterizeSimplePass_RenderThread(PassName, RHICmdList, VertexShaderRef, PixelShaderRef, RenderTargets, DepthStencil, InFunction, RasterizerConfig);
}

void Compushady::Utils::RasterizeSimplePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FVertexShaderRHIRef VertexShaderRef, FPixelShaderRHIRef PixelShaderRef, const TArray<FTextureRHIRef>& RenderTargets, FTextureRHIRef DepthStencil, TFunction<void()> InFunction, const FCompushadyRasterizerConfig& RasterizerConfig)
{
	FRHIRenderPassInfo PassInfo;
	FIntPoint OutputSize;

	if (RenderTargets.Num() == 0)
	{
		// invalid combo?
		if (!DepthStencil)
		{
			return;
		}

		PassInfo = FRHIRenderPassInfo(DepthStencil, EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil);
		OutputSize = DepthStencil->GetSizeXY();
	}
	else
	{
		TArray<FRHITexture*> RTVs;
		for (const FTextureRHIRef& TextureRHI : RenderTargets)
		{
			RTVs.Add(TextureRHI);
		}

		if (DepthStencil)
		{
			PassInfo = FRHIRenderPassInfo(RenderTargets.Num(), RTVs.GetData(), ERenderTargetActions::Load_Store, DepthStencil, EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil);
		}
		else
		{
			PassInfo = FRHIRenderPassInfo(RenderTargets.Num(), RTVs.GetData(), ERenderTargetActions::Load_Store);
		}

		OutputSize = RenderTargets[0]->GetSizeXY();
	}

	RHICmdList.BeginRenderPass(PassInfo, PassName);

	RHICmdList.SetViewport(0, 0, 0.0f, OutputSize.X, OutputSize.Y, 1.0f);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	FillRasterizerPipelineStateInitializer(RasterizerConfig, GraphicsPSOInit);

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShaderRef;
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShaderRef;

	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

	InFunction();

	RHICmdList.EndRenderPass();
}

void Compushady::Utils::RasterizePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FGraphicsPipelineStateInitializer& PipelineStateInitializer, FTextureRHIRef RenderTarget, FTextureRHIRef DepthStencil, TFunction<void()> InFunction)
{
	FRHIRenderPassInfo PassInfo(RenderTarget, ERenderTargetActions::Load_Store);

	if (DepthStencil)
	{
		PassInfo = FRHIRenderPassInfo(RenderTarget, ERenderTargetActions::Load_Store, DepthStencil, EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil);
	}
	RHICmdList.BeginRenderPass(PassInfo, PassName);

	RHICmdList.SetViewport(0, 0, 0.0f, RenderTarget->GetSizeX(), RenderTarget->GetSizeY(), 1.0f);

	RHICmdList.ApplyCachedRenderTargets(PipelineStateInitializer);
	SetGraphicsPipelineState(RHICmdList, PipelineStateInitializer, 0);

	InFunction();

	RHICmdList.EndRenderPass();
}

bool Compushady::Utils::FinalizeShader(TArray<uint8>& ByteCode, const FString& TargetProfile, Compushady::FCompushadyShaderResourceBindings& ShaderResourceBindings, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages, const bool bIsSPIRV)
{
	const ERHIInterfaceType RHIInterfaceType = RHIGetInterfaceType();
	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		if (bIsSPIRV)
		{
			TArray<uint8> HLSL;
			FString EntryPoint;

			if (!Compushady::SPIRVToHLSL(ByteCode, HLSL, EntryPoint, ErrorMessages))
			{
				return false;
			}

			ByteCode.Empty();
			if (!Compushady::CompileHLSL(HLSL, EntryPoint, TargetProfile, ByteCode, ErrorMessages, false))
			{
				return false;
			}

			return FinalizeShader(ByteCode, TargetProfile, ShaderResourceBindings, ResourceBindings, ThreadGroupSize, ErrorMessages, false);
		}

		if (!Compushady::FixupDXIL(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
		{
			return false;
		}
	}
	else if (RHIInterfaceType == ERHIInterfaceType::Vulkan)
	{
		if (!Compushady::FixupSPIRV(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
		{
			return false;
		}
	}
	else
	{
		ErrorMessages = "Unsupported RHI";
		return false;
	}

	return Compushady::Utils::CreateResourceBindings(ShaderResourceBindings, ResourceBindings, ErrorMessages);
}

void Compushady::Utils::FillRasterizerPipelineStateInitializer(const FCompushadyRasterizerConfig& RasterizerConfig, FGraphicsPipelineStateInitializer& PipelineStateInitializer)
{
	if (RasterizerConfig.FillMode == ECompushadyRasterizerFillMode::Solid)
	{
		switch (RasterizerConfig.CullMode)
		{
		case(ECompushadyRasterizerCullMode::None):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
			break;
		case(ECompushadyRasterizerCullMode::ClockWise):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI();
			break;
		case(ECompushadyRasterizerCullMode::CounterClockWise):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI();
			break;
		}
	}
	else if (RasterizerConfig.FillMode == ECompushadyRasterizerFillMode::Wireframe)
	{
		switch (RasterizerConfig.CullMode)
		{
		case(ECompushadyRasterizerCullMode::None):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_None>::GetRHI();
			break;
		case(ECompushadyRasterizerCullMode::ClockWise):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_CW>::GetRHI();
			break;
		case(ECompushadyRasterizerCullMode::CounterClockWise):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_CCW>::GetRHI();
			break;
		}
	}

	if (RasterizerConfig.BlendMode == ECompushadyRasterizerBlendMode::AlphaBlending)
	{
		PipelineStateInitializer.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_InverseDestAlpha, BF_One>::GetRHI();
	}
	else if (RasterizerConfig.BlendMode == ECompushadyRasterizerBlendMode::Always)
	{
		PipelineStateInitializer.BlendState = TStaticBlendState<>::GetRHI();
	}

	if (!RasterizerConfig.bDepthWrite)
	{
		if (!RasterizerConfig.bStencilWrite)
		{
			if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::AlwaysPass)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			}
			else if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::NearOrEqual)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI();
			}
			else if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::FartherOrEqual)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<false, CF_DepthFartherOrEqual>::GetRHI();
			}
		}
		else
		{
			if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::AlwaysPass)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<false, CF_Always, true, CF_Always, SO_Keep, SO_Keep, SO_Replace, true, CF_Always, SO_Keep, SO_Keep, SO_Replace>::GetRHI();
			}
			else if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::NearOrEqual)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<false, CF_DepthNearOrEqual, true, CF_Always, SO_Keep, SO_Keep, SO_Replace, true, CF_Always, SO_Keep, SO_Keep, SO_Replace>::GetRHI();
			}
			else if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::FartherOrEqual)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<false, CF_DepthFartherOrEqual, true, CF_Always, SO_Keep, SO_Keep, SO_Replace, true, CF_Always, SO_Keep, SO_Keep, SO_Replace>::GetRHI();
			}
		}
	}
	else
	{
		if (!RasterizerConfig.bStencilWrite)
		{
			if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::AlwaysPass)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<true, CF_Always>::GetRHI();
			}
			else if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::NearOrEqual)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI();
			}
			else if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::FartherOrEqual)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<true, CF_DepthFartherOrEqual>::GetRHI();
			}
		}
		else
		{
			if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::AlwaysPass)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<true, CF_Always, true, CF_Always, SO_Keep, SO_Keep, SO_Replace, true, CF_Always, SO_Keep, SO_Keep, SO_Replace>::GetRHI();
			}
			else if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::NearOrEqual)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<true, CF_DepthNearOrEqual, true, CF_Always, SO_Keep, SO_Keep, SO_Replace, true, CF_Always, SO_Keep, SO_Keep, SO_Replace>::GetRHI();
			}
			else if (RasterizerConfig.DepthTest == ECompushadyRasterizerDepthTest::FartherOrEqual)
			{
				PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<true, CF_DepthFartherOrEqual, true, CF_Always, SO_Keep, SO_Keep, SO_Replace, true, CF_Always, SO_Keep, SO_Keep, SO_Replace>::GetRHI();
			}
		}
	}

	switch (RasterizerConfig.PrimitiveType)
	{
	case(ECompushadyRasterizerPrimitiveType::TriangleList):
		PipelineStateInitializer.PrimitiveType = PT_TriangleList;
		break;
	case(ECompushadyRasterizerPrimitiveType::LineList):
		PipelineStateInitializer.PrimitiveType = PT_LineList;
		break;
	case(ECompushadyRasterizerPrimitiveType::PointList):
		PipelineStateInitializer.PrimitiveType = PT_PointList;
		break;
	default:
		PipelineStateInitializer.PrimitiveType = PT_TriangleList;
		break;
	}
}

void Compushady::Utils::DrawVertices(FRHICommandList& RHICmdList, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizerConfig& RasterizerConfig)
{
	static int32 Denominators[] = { 3, 2, 1 };
	RHICmdList.DrawPrimitive(0, NumVertices / Denominators[static_cast<int32>(RasterizerConfig.PrimitiveType)], NumInstances);
}