
// Copyright 2023-2024 - Roberto De Ioris.

#include "CompushadyTypes.h"

#include "Misc/Compression.h"

bool Compushady::Utils::GZIPDecompress(const TArray<uint8>& Data, TArray<uint8>& UncompressedData)
{
	// Gzip Compressed ? 10 bytes header and 8 bytes footer
	if (Data.Num() > 18 && Data[0] == 0x1F && Data[1] == 0x8B && Data[2] == 0x08)
	{
		uint32* GzipOriginalSize = (uint32*)(&Data.GetData()[Data.Num() - 4]);
		int64 StartOfBuffer = 10;
		// FEXTRA
		if (Data[3] & 0x04)
		{
			uint16* FExtraXLen = (uint16*)(&Data.GetData()[StartOfBuffer]);
			if (StartOfBuffer + 2 >= Data.Num())
			{
				UE_LOG(LogCompushady, Error, TEXT("Invalid Gzip FEXTRA header."));
				return false;
			}
			StartOfBuffer += 2 + *FExtraXLen;
			if (StartOfBuffer >= Data.Num())
			{
				UE_LOG(LogCompushady, Error, TEXT("Invalid Gzip FEXTRA XLEN."));
				return false;
			}
		}

		// FNAME
		if (Data[3] & 0x08)
		{
			while (Data[StartOfBuffer] != 0)
			{
				StartOfBuffer++;
				if (StartOfBuffer >= Data.Num())
				{
					UE_LOG(LogCompushady, Error, TEXT("Invalid Gzip FNAME header."));
					return false;
				}
			}
			if (++StartOfBuffer >= Data.Num())
			{
				UE_LOG(LogCompushady, Error, TEXT("Invalid Gzip FNAME header."));
				return false;
			}
		}

		// FCOMMENT
		if (Data[3] & 0x10)
		{
			while (Data[StartOfBuffer] != 0)
			{
				StartOfBuffer++;
				if (StartOfBuffer >= Data.Num())
				{
					UE_LOG(LogCompushady, Error, TEXT("Invalid Gzip FCOMMENT header."));
					return false;
				}
			}
			if (++StartOfBuffer >= Data.Num())
			{
				UE_LOG(LogCompushady, Error, TEXT("Invalid Gzip FCOMMENT header."));
				return false;
			}
		}

		// FHCRC
		if (Data[3] & 0x02)
		{
			if (StartOfBuffer + 2 >= Data.Num())
			{
				UE_LOG(LogCompushady, Error, TEXT("Invalid Gzip FHCRC header."));
				return false;
			}
			StartOfBuffer += 2;
		}

		UncompressedData.AddUninitialized(*GzipOriginalSize);
		if (!FCompression::UncompressMemory(NAME_Zlib, UncompressedData.GetData(), *GzipOriginalSize, &Data.GetData()[StartOfBuffer], Data.Num() - StartOfBuffer - 8, COMPRESS_NoFlags, -15))
		{
			UE_LOG(LogCompushady, Error, TEXT("Unable to uncompress Gzip data."));
			return false;
		}

		return true;
	}

	return false;
}