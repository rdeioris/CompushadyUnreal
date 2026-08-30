// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadyAudioFunctionLibrary.h"
#include "CompushadySoundWave.h"
#include "CompushadyAudioSubsystem.h"
#include "CompushadyFunctionLibrary.h"
#include "CompushadySRV.h"
#include "AudioDeviceManager.h"
#include "AudioMixerDevice.h"
#include "AudioBusSubsystem.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"

UCompushadySoundWave* UCompushadyAudioFunctionLibrary::CreateCompushadyUAVSoundWave(const FString& Name, const float Duration, const int32 SampleRate, const int32 NumChannels, UAudioBus* AudioBus)
{
	UCompushadySoundWave* CompushadySoundWave = NewObject<UCompushadySoundWave>();

	CompushadySoundWave->Duration = Duration;
	CompushadySoundWave->SetSampleRate(SampleRate);
	CompushadySoundWave->NumChannels = NumChannels;
	CompushadySoundWave->bLooping = false;

	CompushadySoundWave->UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(Name, SampleRate * Duration * NumChannels * sizeof(float), EPixelFormat::PF_R32_FLOAT);

	if (AudioBus)
	{
		FSoundSourceBusSendInfo BusInfo;
		BusInfo.AudioBus = AudioBus;
		CompushadySoundWave->BusSends.Add(BusInfo);
	}

	return CompushadySoundWave;
}


UCompushadySRV* UCompushadyAudioFunctionLibrary::CreateCompushadySRVAudioTexture2D(UObject* WorldContextObject, const FString& Name, UAudioBus* AudioBus)
{
	if (Audio::FMixerDevice* MixerDevice = FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(WorldContextObject))
	{
		int32 NumBusChannels = AudioBus->GetNumChannels();

		int32 AudioMixerSampleRate = (int32)MixerDevice->GetSampleRate();

		// Start the audio bus (if required)
		uint32 AudioBusId = AudioBus->GetUniqueID();
		int32 NumChannels = (int32)AudioBus->AudioBusChannels + 1;

		UAudioBusSubsystem* AudioBusSubsystem = MixerDevice->GetSubsystem<UAudioBusSubsystem>();
		check(AudioBusSubsystem);
		Audio::FAudioBusKey AudioBusKey = Audio::FAudioBusKey(AudioBusId);
		AudioBusSubsystem->StartAudioBus(AudioBusKey, NumChannels, false);

		// Get an output patch for the audio bus
		int32 NumFramesPerBufferToAnalyze = MixerDevice->GetNumOutputFrames();

		UCompushadySRV* SRV = UCompushadyFunctionLibrary::CreateCompushadySRVTexture2D(Name, NumFramesPerBufferToAnalyze, NumChannels, EPixelFormat::PF_R32_FLOAT);

		if (SRV)
		{
			Audio::FPatchOutputStrongPtr PatchOutputStrongPtr = AudioBusSubsystem->AddPatchOutputForAudioBus(AudioBusKey, NumFramesPerBufferToAnalyze, NumChannels);

			WorldContextObject->GetWorld()->GetSubsystem<UCompushadyAudioSubsystem>()->RegisterAudioTexture(SRV, PatchOutputStrongPtr);

			return SRV;
		}
	}

	return nullptr;
}

bool UCompushadyAudioFunctionLibrary::ReadbackBufferToWAVFileSync(UCompushadyResource* Resource, const FString& Filename, const int64 Offset, const int64 Size, const int32 SampleRate, const int32 NumChannels, const EPixelFormat PixelFormat, const ECompushadyWAVFormat WAVFormat, FString& ErrorMessages)
{
	if (!Resource)
	{
		ErrorMessages = "The resource is NULL";
		return false;
	}

	if (!Resource->IsValidBuffer())
	{
		ErrorMessages = "The resource is not a valid Buffer";
		return false;
	}

	int64 WantedSize = Size;
	if (WantedSize == 0)
	{
		WantedSize = Resource->GetBufferSize();
	}

	if (Offset < 0 || Offset + WantedSize > Resource->GetBufferSize())
	{
		ErrorMessages = "Invalid Buffer offset";
		return false;
	}

	if (!GPixelFormats[PixelFormat].Supported)
	{
		ErrorMessages = FString::Printf(TEXT("Unsupported Pixel Format %s"), GetPixelFormatString(PixelFormat));
		return false;
	}

	if (PixelFormat == EPixelFormat::PF_R32_FLOAT)
	{
		auto WAVWriter = [Filename, NumChannels, SampleRate, Offset, WantedSize](const void* Data)
			{
				TArray<uint8> WAV;
				const uint32 WantedSize32 = WantedSize;
				const uint32 OverallFileSize = 12 + 24 + 8 + WantedSize;
				WAV.Append({ 0x52, 0x49, 0x46, 0x46 }); // RIFF
				WAV.Append(reinterpret_cast<const uint8*>(&OverallFileSize), sizeof(uint32)); // file size - 8
				WAV.Append({ 0x57, 0x41, 0x56, 0x45 }); // WAVE

				WAV.Append({ 0x66, 0x6D, 0x74, 0x20 }); // fmt
				WAV.Append({ 0x10, 0, 0, 0 }); // chunk size (16)
				WAV.Append({ 3, 0 }); // Audio format (float)
				const uint16 NumChannels16 = NumChannels;
				const uint32 SampleRate32 = SampleRate;
				const uint16 BytesPerBlock = sizeof(float) * NumChannels;
				const uint32 BytesPerSec = SampleRate * BytesPerBlock;
				const uint16 BitsPerSample = 32;
				WAV.Append(reinterpret_cast<const uint8*>(&NumChannels16), sizeof(uint16)); // Number of channels
				WAV.Append(reinterpret_cast<const uint8*>(&SampleRate32), sizeof(uint32)); // Sample rate
				WAV.Append(reinterpret_cast<const uint8*>(&BytesPerSec), sizeof(uint32)); // Number of bytes to read per second
				WAV.Append(reinterpret_cast<const uint8*>(&BytesPerBlock), sizeof(uint16)); // Number of bytes per block
				WAV.Append(reinterpret_cast<const uint8*>(&BitsPerSample), sizeof(uint16)); // Number of bits per sample

				WAV.Append({ 0x64, 0x61, 0x74, 0x61 }); // data
				WAV.Append(reinterpret_cast<const uint8*>(&WantedSize32), sizeof(uint32)); // SampledData size

				WAV.Append(reinterpret_cast<const uint8*>(Data) + Offset, WantedSize);

				FFileHelper::SaveArrayToFile(WAV, *Filename);

				return true;
			};
		return Resource->MapReadAndExecuteSync(WAVWriter);
	}

	ErrorMessages = FString::Printf(TEXT("Unsupported Pixel Format %s"), GetPixelFormatString(PixelFormat));
	return false;
}
