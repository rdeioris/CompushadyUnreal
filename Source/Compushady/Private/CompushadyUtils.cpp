// Copyright 2023-2024 - Roberto De Ioris.

#include "CompushadyTypes.h"

bool Compushady::Utils::SplitToFloats(const TArray<FString>& Lines, const TArray<int32>& Columns, const FString& Separator, const int32 SkipLines, const bool bCullEmpty, TArray<float>& Values, int32& Stride)
{
	if (SkipLines > Lines.Num())
	{
		return false;
	}

	FString WantedSeparator = Separator;
	if (WantedSeparator.IsEmpty())
	{
		WantedSeparator = " ";
	}

	Values.AddUninitialized((Lines.Num() - SkipLines) * Columns.Num());

	Stride = sizeof(float) * Columns.Num();

	std::atomic<int32> ProcessedLines = 0;

	ParallelFor(Lines.Num() - SkipLines, [&](const int32 LineIndex)
		{
			const FString& Line = Lines[SkipLines + LineIndex];
			TArray<FString> Items;
			Line.ParseIntoArray(Items, *WantedSeparator, bCullEmpty);

			bool bValid = true;
			for (const int32 ColumnIndex : Columns)
			{
				if (!Items.IsValidIndex(ColumnIndex))
				{
					bValid = false;
					break;
				}
			}

			if (bValid)
			{
				// atomic increment
				ProcessedLines++;

				const int32 Index = ProcessedLines - 1;
				for (int32 ColumnIndexIndex = 0; ColumnIndexIndex < Columns.Num(); ColumnIndexIndex++)
				{
					Values[Index * Columns.Num() + ColumnIndexIndex] = FCString::Atof(*Items[Columns[ColumnIndexIndex]]);
				}
			}
		});

	// do not shrink memory as we are going to trash this array after GPU upload
	Values.SetNum(ProcessedLines * Columns.Num(), false);

	return true;
}

void Compushady::Utils::BytesToStringArray(const TArray<uint8>& Bytes, TArray<FString>& Lines)
{
	FString CurrentLine;
	for (int32 CharIndex = 0; CharIndex < Bytes.Num(); CharIndex++)
	{
		if (Bytes[CharIndex] != '\r' && Bytes[CharIndex] != '\n')
		{
			CurrentLine += static_cast<char>(Bytes[CharIndex]);
		}
		else if (!CurrentLine.IsEmpty())
		{
			Lines.Add(CurrentLine);
			CurrentLine.Empty();
		}
	}

	if (!CurrentLine.IsEmpty())
	{
		Lines.Add(CurrentLine);
	}
}