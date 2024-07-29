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
		});

	QueueAudio(TempData.GetData(), TempData.Num());
}