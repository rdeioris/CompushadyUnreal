// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadyTypes.h"

#define COMPUSHADY_TIFF_TYPE_ASCII 2
#define COMPUSHADY_TIFF_TYPE_SHORT 3
#define COMPUSHADY_TIFF_TYPE_LONG 4
#define COMPUSHADY_TIFF_TYPE_RATIONAL 5

#define COMPUSHADY_TIFF_FORMAT_UINT 1
#define COMPUSHADY_TIFF_FORMAT_INT 2
#define COMPUSHADY_TIFF_FORMAT_FLOAT 3

class FCompushadyTIFFGenerator
{
public:
	FCompushadyTIFFGenerator(TArray<uint8>& InData, const uint16 IFDEntryNum) : Data(InData)
	{
		// add file header
		if (InData.Num() == 0)
		{
			Data.Add(0x49);
			Data.Add(0x49);

			Data.Add(42);
			Data.Add(0);

			Data.Add(8);
			Data.Add(0);
			Data.Add(0);
			Data.Add(0);
		}
#if 0
		else
		{
			// add next IFDEntry
			uint32 CurrentOffset = Data.Num();
			FMemory::Memcpy(Data.GetData() + CurrentOffset - 4, &CurrentOffset, sizeof(uint32));
		}
#endif

		Data.Append(reinterpret_cast<const uint8*>(&IFDEntryNum), sizeof(uint16));

		AdditionalDataOffset = Data.Num() + (12 * IFDEntryNum) + sizeof(uint32);
	}

	void AddIFDEntry(const uint16 Tag, const uint16 Type, const uint32 NumberOfValues, const uint32 ValueOrOffset)
	{
		Data.Append(reinterpret_cast<const uint8*>(&Tag), sizeof(uint16));
		Data.Append(reinterpret_cast<const uint8*>(&Type), sizeof(uint16));
		Data.Append(reinterpret_cast<const uint8*>(&NumberOfValues), sizeof(uint32));
		Data.Append(reinterpret_cast<const uint8*>(&ValueOrOffset), sizeof(uint32));
	}

	uint32 GetCurrentDataOffset() const
	{
		return AdditionalDataOffset + static_cast<uint32>(AdditionalData.Num());
	}

	uint32 AddAdditionalData(const uint32 Value)
	{
		const uint32 CurrentOffset = GetCurrentDataOffset();
		AdditionalData.Append(reinterpret_cast<const uint8*>(&Value), sizeof(uint32));
		return CurrentOffset;
	}

	uint32 AddAdditionalData(const uint16 Value)
	{
		const uint32 CurrentOffset = GetCurrentDataOffset();
		AdditionalData.Append(reinterpret_cast<const uint8*>(&Value), sizeof(uint16));
		return CurrentOffset;
	}

	uint32 AddAdditionalData(const uint8 Value)
	{
		const uint32 CurrentOffset = GetCurrentDataOffset();
		AdditionalData.Add(Value);
		return CurrentOffset;
	}

	uint32 AddAdditionalData(const uint8* RawData, const int32 Num)
	{
		const uint32 CurrentOffset = GetCurrentDataOffset();
		AdditionalData.Append(RawData, Num);
		return CurrentOffset;
	}

	void AddImageWidth(const uint32 Width)
	{
		AddIFDEntry(0x100, COMPUSHADY_TIFF_TYPE_SHORT, 1, Width);
	}

	void AddImageHeight(const uint32 Width)
	{
		AddIFDEntry(0x101, COMPUSHADY_TIFF_TYPE_SHORT, 1, Width);
	}

	void AddBitsPerSample(const uint16 NumComponents, const uint16 BitsPerSample)
	{
		if (NumComponents == 1)
		{
			AddIFDEntry(0x102, COMPUSHADY_TIFF_TYPE_SHORT, NumComponents, BitsPerSample);
			return;
		}

		if (NumComponents == 2)
		{
			AddIFDEntry(0x102, COMPUSHADY_TIFF_TYPE_SHORT, NumComponents, BitsPerSample << 16 | BitsPerSample);
			return;
		}

		const uint32 BitsPerSampleDataOffset = GetCurrentDataOffset();
		for (uint16 ComponentIndex = 0; ComponentIndex < NumComponents; ComponentIndex++)
		{
			AddAdditionalData(BitsPerSample);
		}
		AddIFDEntry(0x102, COMPUSHADY_TIFF_TYPE_SHORT, NumComponents, BitsPerSampleDataOffset);
	}

	void AddCompression()
	{
		AddIFDEntry(0x103, COMPUSHADY_TIFF_TYPE_SHORT, 1, 1);
	}

	void AddPhotometricInterpretation()
	{
		AddIFDEntry(0x106, COMPUSHADY_TIFF_TYPE_SHORT, 1, 2);
	}

	void AddImageDescription(const FString& ImageDescription)
	{
		TArray<uint8> ASCII;
		Compushady::StringToShaderCode(ImageDescription, ASCII);
		ASCII.Add(static_cast<uint8>(0));
	
		if (ASCII.Num() <= 4)
		{
			AddIFDEntry(0x10e, COMPUSHADY_TIFF_TYPE_ASCII, ASCII.Num(), *(reinterpret_cast<const uint32*>(ASCII.GetData())));
			return;
		}

		const uint32 ImageDescriptionDataOffset = AddAdditionalData(ASCII.GetData(), ASCII.Num());
		// align
		if (ASCII.Num() % 2 != 0)
		{
			AddAdditionalData(static_cast<uint8>(0));
		}
		AddIFDEntry(0x10e, COMPUSHADY_TIFF_TYPE_ASCII, ASCII.Num(), ImageDescriptionDataOffset);
	}

	void AddSamplesPerPixel(const uint16 NumSamples)
	{
		AddIFDEntry(0x115, COMPUSHADY_TIFF_TYPE_SHORT, 1, NumSamples);
	}

	void AddRowsPerStrip(const uint32 Height)
	{
		AddIFDEntry(0x116, COMPUSHADY_TIFF_TYPE_SHORT, 1, Height);
	}

	void AddPlanarConfiguration()
	{
		AddIFDEntry(0x11c, COMPUSHADY_TIFF_TYPE_SHORT, 1, 1);
	}

	void AddSampleFormat(const uint16 NumComponents, const uint16 Format)
	{
		if (NumComponents == 1)
		{
			AddIFDEntry(0x153, COMPUSHADY_TIFF_TYPE_SHORT, NumComponents, Format);
			return;
		}

		if (NumComponents == 2)
		{
			AddIFDEntry(0x153, COMPUSHADY_TIFF_TYPE_SHORT, NumComponents, Format << 16 | Format);
			return;
		}

		const uint32 SampleFormatDataOffset = GetCurrentDataOffset();
		for (uint16 ComponentIndex = 0; ComponentIndex < NumComponents; ComponentIndex++)
		{
			AddAdditionalData(Format);
		}
		AddIFDEntry(0x153, COMPUSHADY_TIFF_TYPE_SHORT, NumComponents, SampleFormatDataOffset);
	}

	void AddExtraSamples(const uint16 NumExtra)
	{
		if (NumExtra <= 2)
		{
			AddIFDEntry(0x152, COMPUSHADY_TIFF_TYPE_SHORT, NumExtra, 1);
			return;
		}

		const uint32 ExtraSamplesDataOffset = GetCurrentDataOffset();
		for (uint16 ComponentIndex = 0; ComponentIndex < NumExtra; ComponentIndex++)
		{
			AddAdditionalData(ComponentIndex == 0 ? static_cast<uint16>(1) : static_cast<uint16>(0));
		}
		AddIFDEntry(0x152, COMPUSHADY_TIFF_TYPE_SHORT, NumExtra, ExtraSamplesDataOffset);
	}

	void AddStripByteCounts(const uint32 StripByteCounts)
	{
		AddIFDEntry(0x117, COMPUSHADY_TIFF_TYPE_LONG, 1, StripByteCounts);
	}

	void AddStripOffsets(const void* RawPtr, const uint32 Stride, const uint32 Width, const uint32 Height, const uint32 PixelSize)
	{
		const uint32 ExtraStripOffsetsDataOffset = GetCurrentDataOffset();
		const uint8* Ptr = reinterpret_cast<const uint8*>(RawPtr);
		TArray<uint8> RowData;
		RowData.AddUninitialized(Width * PixelSize);
		for (uint32 Y = 0; Y < Height; Y++)
		{
			FMemory::Memcpy(RowData.GetData(), Ptr, RowData.Num());
			AddAdditionalData(RowData.GetData(), RowData.Num());
			Ptr += Stride;
		}

		AddIFDEntry(0x111, COMPUSHADY_TIFF_TYPE_LONG, 1, ExtraStripOffsetsDataOffset);
	}

	void AddResolutionUnit()
	{
		AddIFDEntry(0x128, COMPUSHADY_TIFF_TYPE_SHORT, 1, 1);
	}

	void AddXResolution(const uint32 Width)
	{
		const uint32 XResolutionDataOffset = GetCurrentDataOffset();
		AddAdditionalData(Width);
		AddAdditionalData(static_cast<const uint32>(1));
		AddIFDEntry(0x11a, COMPUSHADY_TIFF_TYPE_RATIONAL, 1, XResolutionDataOffset);
	}

	void AddYResolution(const uint32 Height)
	{
		const uint32 YResolutionDataOffset = GetCurrentDataOffset();
		AddAdditionalData(Height);
		AddAdditionalData(static_cast<const uint32>(1));
		AddIFDEntry(0x11b, COMPUSHADY_TIFF_TYPE_RATIONAL, 1, YResolutionDataOffset);
	}

	int32 Finalize()
	{
		const int32 NextOffset = Data.Num();
		Data.AddZeroed(4);
		Data.Append(AdditionalData);
		return NextOffset;
	}

protected:
	TArray<uint8>& Data;
	TArray<uint8> AdditionalData;
	uint32 AdditionalDataOffset = 0;
};

bool Compushady::Utils::GenerateTIFF(const void* Data, const int32 Stride, const uint32 Width, const uint32 Height, const EPixelFormat PixelFormat, const FString ImageDescription, TArray<uint8>& IFD)
{
	if (!GPixelFormats[PixelFormat].Supported)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unsupported PixelFormat %d for TIFF generator"), static_cast<int32>(PixelFormat));
		return false;
	}

	const uint16 PixelSize = GPixelFormats[PixelFormat].BlockBytes;
	const uint16 NumComponents = GPixelFormats[PixelFormat].NumComponents;
	const uint16 BitsPerComponent = (PixelSize / NumComponents) * 8;

	uint16 Format = 0;

	switch (PixelFormat)
	{
	case EPixelFormat::PF_FloatRGBA:
	case EPixelFormat::PF_R32G32B32F:
	case EPixelFormat::PF_A32B32G32R32F:
	case EPixelFormat::PF_R32_FLOAT:
	case EPixelFormat::PF_G32R32F:
	case EPixelFormat::PF_R16F:
	case EPixelFormat::PF_G16R16F:
	case EPixelFormat::PF_G16R16F_FILTER:
	case EPixelFormat::PF_R16F_FILTER:
		Format = COMPUSHADY_TIFF_FORMAT_FLOAT;
		break;
	case EPixelFormat::PF_R8G8B8A8:
	case EPixelFormat::PF_R16G16B16A16_UINT:
	case EPixelFormat::PF_R16G16B16A16_UNORM:
	case EPixelFormat::PF_R32G32_UINT:
	case EPixelFormat::PF_R16G16_UINT:
	case EPixelFormat::PF_R8G8B8A8_UINT:
	case EPixelFormat::PF_R32G32B32_UINT:
	case EPixelFormat::PF_G16R16:
	case EPixelFormat::PF_R16_UINT:
	case EPixelFormat::PF_R32_UINT:
	case EPixelFormat::PF_R8G8_UINT:
	case EPixelFormat::PF_R32G32B32A32_UINT:
	case EPixelFormat::PF_A8:
	case EPixelFormat::PF_A16B16G16R16:
	case EPixelFormat::PF_R8_UINT:
		Format = COMPUSHADY_TIFF_FORMAT_UINT;
		break;
	case EPixelFormat::PF_R16G16B16A16_SINT:
	case EPixelFormat::PF_R16G16B16A16_SNORM:
	case EPixelFormat::PF_R8_SINT:
	case EPixelFormat::PF_R8G8B8A8_SNORM:
	case EPixelFormat::PF_R32G32B32_SINT:
	case EPixelFormat::PF_G16R16_SNORM:
	case EPixelFormat::PF_R8:
	case EPixelFormat::PF_R8G8:
	case EPixelFormat::PF_V8U8:
	case EPixelFormat::PF_R16_SINT:
	case EPixelFormat::PF_R32_SINT:
		Format = COMPUSHADY_TIFF_FORMAT_INT;
		break;
	default:
		UE_LOG(LogCompushady, Error, TEXT("Unsupported PixelFormat %d for TIFF generator"), static_cast<int32>(PixelFormat));
		return false;
	}

	uint16 IFDEntryNum = 14;

	if (!ImageDescription.IsEmpty())
	{
		IFDEntryNum++;
	}

	if (NumComponents > 3)
	{
		IFDEntryNum++;
	}

	FCompushadyTIFFGenerator TIFFGenerator(IFD, IFDEntryNum);

	// 0100 ImageWidth
	TIFFGenerator.AddImageWidth(Width);

	// 0101 ImageHeight
	TIFFGenerator.AddImageHeight(Height);

	// 0102 BitsPerSample
	TIFFGenerator.AddBitsPerSample(NumComponents, BitsPerComponent);

	// 0103 Compression
	TIFFGenerator.AddCompression();

	// 0106 PhotometricInterpretation
	TIFFGenerator.AddPhotometricInterpretation();

	// 10e ImageDescription
	if (!ImageDescription.IsEmpty())
	{
		TIFFGenerator.AddImageDescription(ImageDescription);
	}

	// 111 StripOffsets
	TIFFGenerator.AddStripOffsets(Data, Stride, Width, Height, PixelSize);

	// 115 SamplesPerPixel
	TIFFGenerator.AddSamplesPerPixel(NumComponents);

	// 116 RowsPerStrip
	TIFFGenerator.AddRowsPerStrip(Height);

	// 117 StripByteCounts
	TIFFGenerator.AddStripByteCounts(Width * Height * PixelSize);

	// 11a XResolution
	TIFFGenerator.AddXResolution(Width);

	// 11b YResolution
	TIFFGenerator.AddYResolution(Height);

	// 11c PlanarConfiguration
	TIFFGenerator.AddPlanarConfiguration();

	// 128 ResolutionUnit
	TIFFGenerator.AddResolutionUnit();

	// 152 ExtraSamples
	if (NumComponents > 3)
	{
		TIFFGenerator.AddExtraSamples(NumComponents - 3);
	}

	// 153 SampleFormat
	TIFFGenerator.AddSampleFormat(NumComponents, Format);

	TIFFGenerator.Finalize();

	return true;
}