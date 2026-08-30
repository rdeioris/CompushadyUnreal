// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"                 // EPixelFormat, GPixelFormats
#include "Misc/FileHelper.h"
#include "Misc/Compression.h"
#include "Compushady.h"          // Compushady::StringToShaderCode / ShaderCodeToString (COMPUSHADY_API)

// Module-local log category (core's LogCompushady is not exported across modules).
DECLARE_LOG_CATEGORY_EXTERN(LogCompushadyFileFormats, Log, All);

// File-format parsers/encoders relocated out of the core Compushady module.
// The namespaces are kept (Compushady::Utils / Compushady::PointCloud) to minimise churn in the
// parser .cpp files; only the export macro changes to COMPUSHADYFILEFORMATS_API.
namespace Compushady
{
	namespace Utils
	{
		COMPUSHADYFILEFORMATS_API bool GenerateTIFF(const void* Data, const int32 Stride, const uint32 Width, const uint32 Height, const EPixelFormat PixelFormat, const FString ImageDescription, TArray<uint8>& IFD);
		COMPUSHADYFILEFORMATS_API bool LoadNRRD(const FString& Filename, TArray64<uint8>& SlicesData, int64& Offset, uint32& Width, uint32& Height, uint32& Depth, EPixelFormat& PixelFormat);
		COMPUSHADYFILEFORMATS_API bool GZIPDecompress(const TArray<uint8>& Data, TArray<uint8>& UncompressedData);
	}

	namespace PointCloud
	{
		COMPUSHADYFILEFORMATS_API bool LoadLASToFloatArray(const TArray<uint8>& Data, TArray<float>& Floats, const bool bIncludeColors);
	}
}
