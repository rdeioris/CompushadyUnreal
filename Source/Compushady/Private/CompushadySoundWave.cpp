// Copyright 2023 - Roberto De Ioris.


#include "CompushadySoundWave.h"

int32 UCompushadySoundWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	UE_LOG(LogTemp, Error, TEXT("Generating for %d %d"), OutAudio.Num(), NumSamples);
	return 0;
}