// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CompushadyTypes.h"
#include "CompushadyAudioFunctionLibrary.generated.h"

class UCompushadyResource;
class UCompushadySRV;
class UCompushadySoundWave;
class UAudioBus;

/**
 * Audio Blueprint nodes relocated out of the core Compushady module: the SoundWave / audio-bus
 * factories (previously on UCompushadyFunctionLibrary) and the WAV readback (previously on
 * UCompushadyResource). Moving these lets the core module drop the AudioExtensions / AudioMixer /
 * SignalProcessing dependencies.
 */
UCLASS()
class COMPUSHADYAUDIO_API UCompushadyAudioFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Creates a procedural SoundWave backed by a float UAV buffer sized for the given Duration/SampleRate/NumChannels, optionally routed to an AudioBus.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	static UCompushadySoundWave* CreateCompushadyUAVSoundWave(const FString& Name, const float Duration, const int32 SampleRate = 48000, const int32 NumChannels = 2, UAudioBus* AudioBus = nullptr);

	// Starts the given AudioBus and returns an SRV over a 2D texture that is continuously fed the bus output audio frames.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	static UCompushadySRV* CreateCompushadySRVAudioTexture2D(UObject* WorldContextObject, const FString& Name, UAudioBus* AudioBus);

	// Reads back a float buffer resource (a Size-byte range starting at Offset) and writes it to disk as a WAV file with the given SampleRate/NumChannels, returning true on success.
	UFUNCTION(BlueprintCallable, Category = "Compushady|Audio")
	static bool ReadbackBufferToWAVFileSync(UCompushadyResource* Resource, const FString& Filename, const int64 Offset, const int64 Size, const int32 SampleRate, const int32 NumChannels, const EPixelFormat PixelFormat, const ECompushadyWAVFormat WAVFormat, FString& ErrorMessages);
};
