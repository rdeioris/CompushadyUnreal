// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadySoundWave.h"

void UCompushadySoundWave::UpdateSamples()
{
	if (!UAV)
	{
		return;
	}

	if (TempData.Num() != UAV->GetBufferSize())
	{
		TempData.Empty(UAV->GetBufferSize());
		TempData.AddUninitialized(UAV->GetBufferSize());
	}

	UAV->MapReadAndExecuteSync([this](const void* Data)
		{
			FMemory::Memcpy(TempData.GetData(), Data, UAV->GetBufferSize());
			return true;
		});

	QueueAudio(TempData.GetData(), TempData.Num());
}

int32 UCompushadySoundWave::GetSampleRate() const
{
	return SampleRate;
}
int32 UCompushadySoundWave::GetNumChanels() const
{
	return NumChannels;
}
float UCompushadySoundWave::GetDuration() const
{
	return Duration;
}
int32 UCompushadySoundWave::GetSamples() const
{
	return Duration * SampleRate;
}

int32 UCompushadySoundWave::GetTotalSamples() const
{
	return Duration * SampleRate * NumChannels;
}