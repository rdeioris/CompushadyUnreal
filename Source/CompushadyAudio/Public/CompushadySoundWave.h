// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include "CompushadyUAV.h"
#include "CompushadySoundWave.generated.h"

/**
 *
 */
UCLASS()
class COMPUSHADYAUDIO_API UCompushadySoundWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	Audio::EAudioMixerStreamDataFormat::Type GetGeneratedPCMDataFormat() const override { return Audio::EAudioMixerStreamDataFormat::Float; }

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	UCompushadyUAV* UAV;

	// Copies the latest contents of the bound UAV buffer and queues them as audio samples for playback.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void UpdateSamples();


	// Returns the sample rate (in Hz) of this SoundWave.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetSampleRate() const;

	// Returns the number of audio channels of this SoundWave.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetNumChanels() const;

	// Returns the duration (in seconds) of this SoundWave.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	float GetDuration() const;

	// Returns the number of samples per channel (Duration times SampleRate).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetSamples() const;

	// Returns the total number of samples across all channels (Duration times SampleRate times NumChannels).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetTotalSamples() const;

	// Resets the SoundWave, discarding any audio samples currently queued for playback.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void ClearSamples();

	// Returns true while there are still queued audio bytes left to play.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	bool IsPlaying();
protected:
	TArray<uint8> TempData;
};
