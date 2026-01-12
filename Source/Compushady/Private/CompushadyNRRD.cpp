// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadyTypes.h"
#include "Misc/FileHelper.h"

bool Compushady::Utils::LoadNRRD(const FString& Filename, TArray64<uint8>& SlicesData, int64& Offset, uint32& Width, uint32& Height, uint32& Depth, EPixelFormat& PixelFormat)
{
	if (!FFileHelper::LoadFileToArray(SlicesData, *Filename))
	{
		UE_LOG(LogCompushady, Error, TEXT("Unable to load file %s"), *Filename);
		return false;
	}

	TArray<FString> Lines;
	TArray<uint8> Line;
	int64 Index = 0;
	for (Index = 0; Index < SlicesData.Num(); Index++)
	{
		const uint8 Char = SlicesData[Index];
		if (Char == '\n')
		{
			const FString StringLine = Compushady::ShaderCodeToString(Line).TrimStartAndEnd();
			if (StringLine.IsEmpty())
			{
				break;
			}
			Lines.Add(StringLine);
			Line.Empty();
		}
		else if (Char != '\r')
		{
			Line.Add(Char);
		}
	}

	if (Lines.Num() < 5 || !Lines[0].StartsWith("NRRD"))
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD file"));
		return false;
	}

	PixelFormat = EPixelFormat::PF_Unknown;
	int32 Dimension = 0;
	Width = 0;
	Height = 0;
	Depth = 0;
	TArray<FString> Sizes;

	for (int32 LineIndex = 1; LineIndex < Lines.Num(); LineIndex++)
	{
		const FString NRRDHeaderLine = Lines[LineIndex];

		int32 FoundIndex = 0;
		if (NRRDHeaderLine.FindChar(':', FoundIndex))
		{
			if (NRRDHeaderLine.IsValidIndex(FoundIndex + 1) && NRRDHeaderLine[FoundIndex + 1] == '=')
			{
				FoundIndex++;
			}

			const FString Key = NRRDHeaderLine.Left(FoundIndex).TrimStartAndEnd();
			const FString Value = NRRDHeaderLine.Mid(FoundIndex + 1).TrimStartAndEnd();

			if (Key == "type")
			{
				if (Value == "signed char" || Value == "int8" || Value == "int8_t")
				{
					PixelFormat = EPixelFormat::PF_R8_SINT;
				}
				else if (Value == "uchar" || Value == "unsigned char" || Value == "uint8" || Value == "uint8_t")
				{
					PixelFormat = EPixelFormat::PF_R8_UINT;
				}
				else if (Value == "short" || Value == "short int" || Value == "signed short" || Value == "signed short int" || Value == "int16" || Value == "int16_t")
				{
					PixelFormat = EPixelFormat::PF_R16_SINT;
				}
				else if (Value == "ushort" || Value == "unsigned short" || Value == "unsigned short int" || Value == "uint16" || Value == "uint16_t")
				{
					PixelFormat = EPixelFormat::PF_R16_UINT;
				}
				else if (Value == "int" || Value == "signed int" || Value == "int32" || Value == "int32_t")
				{
					PixelFormat = EPixelFormat::PF_R32_SINT;
				}
				else if (Value == "uint" || Value == "unsigned int" || Value == "uint32" || Value == "uint32_t")
				{
					PixelFormat = EPixelFormat::PF_R32_UINT;
				}
				else if (Value == "float")
				{
					PixelFormat = EPixelFormat::PF_R32_FLOAT;
				}
				else
				{
					UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD type"));
					return false;
				}
			}
			else if (Key == "dimension")
			{
				Dimension = FCString::Atoi(*Value);
				if (Dimension < 1)
				{
					UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD dimension"));
					return false;
				}
			}
			else if (Key == "encoding")
			{
				if (Value != "raw")
				{
					UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD encoding"));
					return false;
				}
			}
			else if (Key == "sizes")
			{
				const TCHAR* Delimiters[] = { TEXT(" "), TEXT("\t") };
				Value.ParseIntoArray(Sizes, Delimiters, 2, false);
			}
		}
	}

	if (PixelFormat == EPixelFormat::PF_Unknown)
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD: missing type"));
		return false;
	}

	if (Dimension == 0)
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD: missing dimension"));
		return false;
	}

	if (Sizes.Num() == 0)
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD: missing sizes"));
		return false;
	}

	Width = FCString::Atoi(*Sizes[0]);
	Height = 1;
	Depth = 1;
	if (Sizes.IsValidIndex(1))
	{
		Height = FCString::Atoi(*Sizes[1]);
	}
	if (Sizes.IsValidIndex(2))
	{
		Depth = FCString::Atoi(*Sizes[2]);
	}

	if (Width <= 0 || Height <= 0 || Depth <= 0)
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD sizes (%d %d %d)"), Width, Height, Depth);
		return false;
	}

	Offset = Index + 1;

	const int64 RequiredSize = Width * Height * Depth * GPixelFormats[PixelFormat].BlockBytes;

	if (Offset >= SlicesData.Num() || (SlicesData.Num() - Offset) < RequiredSize)
	{
		UE_LOG(LogCompushady, Error, TEXT("Invalid NRRD file size (required: %lld bytes)"), RequiredSize);
		return false;
	}

	return true;
}