// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include "CompushadySoundWave.generated.h"

/**
 * 
 */
UCLASS()
class COMPUSHADY_API UCompushadySoundWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;
	
	virtual Audio::EAudioMixerStreamDataFormat::Type GetGeneratedPCMDataFormat() const override { return Audio::EAudioMixerStreamDataFormat::Float; }

protected:
	float GeneratedSeconds;
};
