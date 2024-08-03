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
class COMPUSHADY_API UCompushadySoundWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	Audio::EAudioMixerStreamDataFormat::Type GetGeneratedPCMDataFormat() const override { return Audio::EAudioMixerStreamDataFormat::Float; }

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	UCompushadyUAV* UAV;

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void UpdateSamples();


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetSampleRate() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetNumChanels() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	float GetDuration() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetSamples() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetTotalSamples() const;
protected:
	TArray<uint8> TempData;
};
