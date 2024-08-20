// Copyright 2023-2024 - Roberto De Ioris.

#include "CompushadyTypes.h"

bool Compushady::PointCloud::LoadLASToFloatArray(const TArray<uint8>& Data, TArray<float>& Floats, const bool bIncludeColors)
{
	// ensure HeaderSize and Offset to Point Data are available
	if (Data.Num() < 179)
	{
		return false;
	}

	if (Data[0] != 'L' || Data[1] != 'A' || Data[2] != 'S' || Data[3] != 'F')
	{
		return false;
	}

	uint32 NumberOfPoints = 0;

	const uint8 VersionMajor = Data[24];
	const uint8 VersionMinor = Data[25];

	const uint16 HeaderSize = *(reinterpret_cast<const uint16*>(Data.GetData() + 94));
	const uint32 OffsetToPointData = *(reinterpret_cast<const uint32*>(Data.GetData() + 96));

	const uint8 RecordFormat = Data[104];
	const uint16 RecordLength = *(reinterpret_cast<const uint16*>(Data.GetData() + 105));

	const uint32 LegacyNumberOfPoints = *(reinterpret_cast<const uint32*>(Data.GetData() + 107));

	NumberOfPoints = LegacyNumberOfPoints;

	const double XScaleFactor = *(reinterpret_cast<const double*>(Data.GetData() + 131));
	const double YScaleFactor = *(reinterpret_cast<const double*>(Data.GetData() + 139));
	const double ZScaleFactor = *(reinterpret_cast<const double*>(Data.GetData() + 147));

	const double XOffset = *(reinterpret_cast<const double*>(Data.GetData() + 155));
	const double YOffset = *(reinterpret_cast<const double*>(Data.GetData() + 163));
	const double ZOffset = *(reinterpret_cast<const double*>(Data.GetData() + 170));

	if (HeaderSize >= 375 && VersionMajor >= 1 && VersionMinor >= 4)
	{
		NumberOfPoints = *(reinterpret_cast<const uint64*>(Data.GetData() + 247));
	}

	if ((OffsetToPointData + NumberOfPoints * RecordLength) > static_cast<uint32>(Data.Num()))
	{
		return false;
	}

	if (bIncludeColors)
	{
		Floats.AddUninitialized(NumberOfPoints * 6);

		ParallelFor(NumberOfPoints, [&](const uint32 Index)
			{
				const uint8* Ptr = Data.GetData() + OffsetToPointData + (Index * RecordLength);

				const int32 X = *(reinterpret_cast<const int32*>(Ptr));
				const int32 Y = *(reinterpret_cast<const int32*>(Ptr + sizeof(int32)));
				const int32 Z = *(reinterpret_cast<const int32*>(Ptr + sizeof(int32) + sizeof(int32)));

				Floats[Index * 6] = X * XScaleFactor + XOffset;
				Floats[Index * 6 + 1] = Y * YScaleFactor + YOffset;
				Floats[Index * 6 + 2] = Z * ZScaleFactor + ZOffset;

				float R = 0;
				float G = 0;
				float B = 0;

				switch (RecordFormat)
				{
				case 2:
					R = (*(reinterpret_cast<const uint16*>(Ptr + 20))) / 65535.0;
					G = (*(reinterpret_cast<const uint16*>(Ptr + 22))) / 65535.0;
					B = (*(reinterpret_cast<const uint16*>(Ptr + 24))) / 65535.0;
					break;
				case 3:
				case 5:
					R = (*(reinterpret_cast<const uint16*>(Ptr + 28))) / 65535.0;
					G = (*(reinterpret_cast<const uint16*>(Ptr + 30))) / 65535.0;
					B = (*(reinterpret_cast<const uint16*>(Ptr + 32))) / 65535.0;
					break;
				case 7:
				case 8:
				case 10:
					R = (*(reinterpret_cast<const uint16*>(Ptr + 30))) / 65535.0;
					G = (*(reinterpret_cast<const uint16*>(Ptr + 32))) / 65535.0;
					B = (*(reinterpret_cast<const uint16*>(Ptr + 34))) / 65535.0;
					break;
				default:
					break;
				}

				Floats[Index * 6 + 3] = R;
				Floats[Index * 6 + 4] = G;
				Floats[Index * 6 + 5] = B;
			});
	}
	else
	{
		Floats.AddUninitialized(NumberOfPoints * 3);

		ParallelFor(NumberOfPoints, [&](const uint32 Index)
			{
				const uint8* Ptr = Data.GetData() + OffsetToPointData + (Index * RecordLength);

				const int32 X = *(reinterpret_cast<const uint32*>(Ptr));
				const int32 Y = *(reinterpret_cast<const uint32*>(Ptr + sizeof(int32)));
				const int32 Z = *(reinterpret_cast<const uint32*>(Ptr + sizeof(int32) + sizeof(int32)));

				Floats[Index * 3] = X * XScaleFactor + XOffset;
				Floats[Index * 3 + 1] = Y * YScaleFactor + YOffset;
				Floats[Index * 3 + 2] = Z * ZScaleFactor + ZOffset;
			});
	}

	return true;
}