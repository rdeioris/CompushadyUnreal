// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadyFileFormatsFunctionLibrary.h"
#include "CompushadyFileFormats.h"   // Compushady::Utils::GenerateTIFF/LoadNRRD/GZIPDecompress, PointCloud::LoadLASToFloatArray
#include "CompushadyTypes.h"         // UCompushadyResource, Compushady::Utils::BytesToStringArray
#include "CompushadySRV.h"

#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

UCompushadySRV* UCompushadyFileFormatsFunctionLibrary::CreateCompushadySRVTexture2DFromImageFile(const FString& Name, const FString& Filename)
{
	TArray<uint8> ImageData;
	if (!FFileHelper::LoadFileToArray(ImageData, *Filename))
	{
		return nullptr;
	}
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(ImageData.GetData(), ImageData.Num());
	if (ImageFormat == EImageFormat::Invalid)
	{
		return nullptr;
	}

	ERGBFormat RGBFormat = ERGBFormat::BGRA;
	EPixelFormat PixelFormat = EPixelFormat::PF_B8G8R8A8;
	int32 BitDepth = 8;

	if (ImageFormat == EImageFormat::EXR)
	{
		RGBFormat = ERGBFormat::RGBAF;
		BitDepth = 16;
		PixelFormat = EPixelFormat::PF_FloatRGBA;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (!ImageWrapper.IsValid())
	{
		return nullptr;
	}

	if (!ImageWrapper->SetCompressed(ImageData.GetData(), ImageData.Num()))
	{
		return nullptr;
	}

	TArray<uint8> UncompressedBytes;
	if (!ImageWrapper->GetRaw(RGBFormat, BitDepth, UncompressedBytes))
	{
		return nullptr;
	}

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create2D(*Name, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PixelFormat);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource);
	FTextureRHIRef TextureRHIRef = nullptr;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
	{
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(DoCompushadyUpdateTexture2D)(
		[TextureRHIRef, ImageWrapper, &UncompressedBytes, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			FUpdateTextureRegion2D UpdateTextureRegion2D(0, 0, 0, 0, ImageWrapper->GetWidth(), ImageWrapper->GetHeight());
			RHICmdList.UpdateTexture2D(TextureRHIRef, 0, UpdateTextureRegion2D, ImageWrapper->GetWidth() * GPixelFormats[PixelFormat].BlockBytes, UncompressedBytes.GetData());
		});

	FlushRenderingCommands();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(TextureRHIRef))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFileFormatsFunctionLibrary::CreateCompushadySRVTexture3DFromNRRDFile(const FString& Name, const FString& Filename)
{
	TArray64<uint8> SlicesData;
	int64 Offset;
	uint32 Width;
	uint32 Height;
	uint32 Depth;
	EPixelFormat PixelFormat = EPixelFormat::PF_Unknown;
	if (!Compushady::Utils::LoadNRRD(Filename, SlicesData, Offset, Width, Height, Depth, PixelFormat))
	{
		return nullptr;
	}

	FTextureRHIRef TextureRHIRef = nullptr;

	FRHITextureCreateDesc TextureCreateDesc = FRHITextureCreateDesc::Create3D(*Name, Width, Height, Depth, PixelFormat);
	TextureCreateDesc.SetFlags(ETextureCreateFlags::ShaderResource);

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateTexture)(
		[&TextureRHIRef, &TextureCreateDesc](FRHICommandListImmediate& RHICmdList)
		{
			TextureRHIRef = RHICreateTexture(TextureCreateDesc);
		});

	FlushRenderingCommands();

	if (!TextureRHIRef.IsValid() || !TextureRHIRef->IsValid())
	{
		return nullptr;
	}

	ENQUEUE_RENDER_COMMAND(DoCompushadyUpdateTexture3D)(
		[TextureRHIRef, &SlicesData, Offset, Width, Height, Depth, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			FUpdateTextureRegion3D UpdateTextureRegion3D(0, 0, 0, 0, 0, 0, Width, Height, Depth);
			RHICmdList.UpdateTexture3D(TextureRHIRef, 0, UpdateTextureRegion3D, Width * GPixelFormats[PixelFormat].BlockBytes, Width * Height * GPixelFormats[PixelFormat].BlockBytes, SlicesData.GetData() + Offset);
		});

	FlushRenderingCommands();

	UCompushadySRV* CompushadySRV = NewObject<UCompushadySRV>();
	if (!CompushadySRV->InitializeFromTexture(TextureRHIRef))
	{
		return nullptr;
	}

	return CompushadySRV;
}

UCompushadySRV* UCompushadyFileFormatsFunctionLibrary::CreateCompushadySRVBufferFromGZFile(const FString& Name, const FString& Filename, const EPixelFormat PixelFormat)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}

	TArray<uint8> UncompressedData;
	if (!Compushady::Utils::GZIPDecompress(Data, UncompressedData))
	{
		return nullptr;
	}

	return UCompushadyFunctionLibrary::CreateCompushadySRVBufferFromByteArray(Name, UncompressedData, PixelFormat);
}

UCompushadySRV* UCompushadyFileFormatsFunctionLibrary::CreateCompushadySRVStructuredBufferFromGZFile(const FString& Name, const FString& Filename, const int32 Stride, const int64 Offset)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}

	TArray<uint8> UncompressedData;
	if (!Compushady::Utils::GZIPDecompress(Data, UncompressedData))
	{
		return nullptr;
	}

	// NOTE: fixed while relocating - the original passed the still-compressed 'Data'.
	return UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromByteArray(Name, UncompressedData, Stride, Offset);
}

UCompushadySRV* UCompushadyFileFormatsFunctionLibrary::CreateCompushadySRVStructuredBufferFromLASFile(const FString& Name, const FString& Filename, const bool bIncludeColors)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}

	TArray<float> Floats;
	if (!Compushady::PointCloud::LoadLASToFloatArray(Data, Floats, bIncludeColors))
	{
		return nullptr;
	}

	return UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromFloatArray(Name, Floats, bIncludeColors ? (sizeof(float) * 6) : (sizeof(float) * 3));
}

UCompushadySRV* UCompushadyFileFormatsFunctionLibrary::CreateCompushadySRVStructuredBufferFromGZASCIIFile(const FString& Name, const FString& Filename, const TArray<int32>& Columns, const FString& Separator, const int32 SkipLines, const bool bCullEmpty)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}

	TArray<uint8> UncompressedData;
	if (!Compushady::Utils::GZIPDecompress(Data, UncompressedData))
	{
		return nullptr;
	}

	TArray<FString> Lines;
	Compushady::Utils::BytesToStringArray(UncompressedData, Lines);

	return UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromStringArray(Name, Lines, Columns, Separator, SkipLines, bCullEmpty);
}

void UCompushadyFileFormatsFunctionLibrary::CreateCompushadySRVStructuredBufferFromGZASCIIFileAsync(const FString& Name, const FString& Filename, const TArray<int32>& Columns, const FCompushadyResourceCreation& OnResource, const FString& Separator, const int32 SkipLines, const bool bCullEmpty)
{
	UCompushadyFunctionLibrary::CompushadyAsyncResource<TPair<TArray<float>, int32>>(OnResource, [Name, Filename, Columns, OnResource, Separator, SkipLines, bCullEmpty](FString& ErrorMessages)
		{
			TPair<bool, TPair<TArray<float>, int32>> ErrorPair(false, TPair<TArray<float>, int32>());
			TArray<float> Values;

			TArray<uint8> Data;
			if (!FFileHelper::LoadFileToArray(Data, *Filename))
			{
				return ErrorPair;
			}

			TArray<uint8> UncompressedData;
			if (!Compushady::Utils::GZIPDecompress(Data, UncompressedData))
			{
				return ErrorPair;
			}

			TArray<FString> Lines;
			Compushady::Utils::BytesToStringArray(UncompressedData, Lines);

			int32 Stride;
			if (!Compushady::Utils::SplitToFloats(Lines, Columns, Separator, SkipLines, bCullEmpty, Values, Stride))
			{
				return ErrorPair;
			}

			return TPair<bool, TPair<TArray<float>, int32>>(true, TPair<TArray<float>, int32>(Values, Stride));
		},
		[Name](const TPair<TArray<float>, int32>& Pair, FString& ErrorMessages) -> UCompushadyResource*
		{
			if (Pair.Value < 0)
			{
				return nullptr;
			}
			return UCompushadyFunctionLibrary::CreateCompushadySRVStructuredBufferFromFloatArray(Name, Pair.Key, Pair.Value);
		}
	);
}

bool UCompushadyFileFormatsFunctionLibrary::ReadbackTextureToPNGFile(UCompushadyResource* Resource, const FString& Filename, FString& ErrorMessages)
{
	if (!Resource)
	{
		ErrorMessages = "The resource is NULL";
		return false;
	}

	if (!Resource->IsValidTexture())
	{
		ErrorMessages = "The resource is not a valid Texture";
		return false;
	}

	if (Resource->GetTextureRHI()->GetDesc().Format != EPixelFormat::PF_B8G8R8A8 && Resource->GetTextureRHI()->GetDesc().Format != EPixelFormat::PF_R8G8B8A8)
	{
		ErrorMessages = "Only RGBA8 and BGRA8 textures are supported";
		return false;
	}

	auto PngWriter = [Resource, Filename](const void* Data, const uint32 Stride)
		{
			const FIntVector Size = Resource->GetTextureSize();
			IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

			TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
			if (ImageWrapper.IsValid())
			{
				if (ImageWrapper->SetRaw(Data, Size.X * Size.Y * GPixelFormats[Resource->GetTextureRHI()->GetDesc().Format].BlockBytes,
					Size.X, Size.Y, Resource->GetTextureRHI()->GetDesc().Format == EPixelFormat::PF_B8G8R8A8 ? ERGBFormat::BGRA : ERGBFormat::RGBA, 8))
				{
					TArray64<uint8> CompressedBytes = ImageWrapper->GetCompressed();
					FFileHelper::SaveArrayToFile(CompressedBytes, *Filename);
				}
			}
		};

	return Resource->MapTextureSliceAndExecuteSync(PngWriter, 0);
}

bool UCompushadyFileFormatsFunctionLibrary::ReadbackTextureToTIFFFile(UCompushadyResource* Resource, const FString& Filename, const FString& ImageDescription, FString& ErrorMessages)
{
	if (!Resource)
	{
		ErrorMessages = "The resource is NULL";
		return false;
	}

	if (!Resource->IsValidTexture())
	{
		ErrorMessages = "The resource is not a valid Texture";
		return false;
	}

	auto TIFFWriter = [Resource, Filename, ImageDescription](const void* Data, const uint32 Stride)
		{
			const FIntVector Size = Resource->GetTextureSize();
			const EPixelFormat PixelFormat = Resource->GetTextureRHI()->GetDesc().Format;
			TArray<uint8> TIFF;
			if (Compushady::Utils::GenerateTIFF(Data, Stride, Size.X, Size.Y, PixelFormat, ImageDescription, TIFF))
			{
				FFileHelper::SaveArrayToFile(TIFF, *Filename);
			}
		};

	return Resource->MapTextureSliceAndExecuteSync(TIFFWriter, 0);
}

void UCompushadyFileFormatsFunctionLibrary::ReadbackTextureToPNGFileAsync(UCompushadyResource* Resource, const FString& Filename, const FCompushadySignaled& OnSignaled)
{
	if (!Resource)
	{
		OnSignaled.ExecuteIfBound(false, "The resource is NULL");
		return;
	}

	if (!Resource->IsValidTexture())
	{
		OnSignaled.ExecuteIfBound(false, "The resource is not a valid Texture");
		return;
	}

	if (Resource->GetTextureRHI()->GetDesc().Format != EPixelFormat::PF_B8G8R8A8 && Resource->GetTextureRHI()->GetDesc().Format != EPixelFormat::PF_R8G8B8A8)
	{
		OnSignaled.ExecuteIfBound(false, "Only RGBA8 and BGRA8 textures are supported");
		return;
	}

	auto PngWriter = [Resource, Filename](const void* Data, const int32 Stride)
		{
			const FIntVector Size = Resource->GetTextureSize();
			IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
			TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
			if (ImageWrapper.IsValid())
			{
				if (ImageWrapper->SetRaw(Data, Size.X * Size.Y * GPixelFormats[Resource->GetTextureRHI()->GetDesc().Format].BlockBytes,
					Size.X, Size.Y, Resource->GetTextureRHI()->GetDesc().Format == EPixelFormat::PF_B8G8R8A8 ? ERGBFormat::BGRA : ERGBFormat::RGBA, 8))
				{
					TArray64<uint8> CompressedBytes = ImageWrapper->GetCompressed();
					FFileHelper::SaveArrayToFile(CompressedBytes, *Filename);
				}
			}
		};

	Resource->MapTextureSliceAndExecute(PngWriter, 0, OnSignaled);
}

void UCompushadyFileFormatsFunctionLibrary::ReadbackTextureToTIFFFileAsync(UCompushadyResource* Resource, const FString& Filename, const FString& ImageDescription, const FCompushadySignaled& OnSignaled)
{
	if (!Resource)
	{
		OnSignaled.ExecuteIfBound(false, "The resource is NULL");
		return;
	}

	if (!Resource->IsValidTexture())
	{
		OnSignaled.ExecuteIfBound(false, "The resource is not a valid Texture");
		return;
	}

	auto TIFFWriter = [Resource, Filename, ImageDescription](const void* Data, const int32 Stride)
		{
			const FIntVector Size = Resource->GetTextureSize();
			const EPixelFormat PixelFormat = Resource->GetTextureRHI()->GetDesc().Format;
			TArray<uint8> TIFF;
			if (Compushady::Utils::GenerateTIFF(Data, Stride, Size.X, Size.Y, PixelFormat, ImageDescription, TIFF))
			{
				FFileHelper::SaveArrayToFile(TIFF, *Filename);
			}
		};

	Resource->MapTextureSliceAndExecute(TIFFWriter, 0, OnSignaled);
}
