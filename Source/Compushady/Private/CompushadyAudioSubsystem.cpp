// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadyAudioSubsystem.h"
#include "AudioDeviceManager.h"
#include "AudioMixerDevice.h"
#include "Engine/World.h"

bool UCompushadyAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* CurrentWorld = Cast<UWorld>(Outer);
	if (CurrentWorld && (CurrentWorld->WorldType == EWorldType::Game || CurrentWorld->WorldType == EWorldType::PIE || CurrentWorld->WorldType == EWorldType::GamePreview))
	{
		return (RHIGetInterfaceType() == ERHIInterfaceType::D3D12 || RHIGetInterfaceType() == ERHIInterfaceType::Vulkan);
	}
	return false;
}

void UCompushadyAudioSubsystem::Tick(float DeltaTime)
{
	// first check for dead textures
	TArray<TWeakObjectPtr<UCompushadyResource>> DeadAudioTextures;

	for (const TPair<TWeakObjectPtr<UCompushadyResource>, Audio::FPatchOutputStrongPtr>& Pair : RegisteredAudioTextures)
	{
		if (!Pair.Key.IsValid())
		{
			DeadAudioTextures.Add(Pair.Key);
		}
	}

	for (const TWeakObjectPtr<UCompushadyResource>& DeadAudioTexture : DeadAudioTextures)
	{
		RegisteredAudioTextures.Remove(DeadAudioTexture);
	}

	for (const TPair<TWeakObjectPtr<UCompushadyResource>, Audio::FPatchOutputStrongPtr>& Pair : RegisteredAudioTextures)
	{
		if (Pair.Value->GetNumSamplesAvailable() >= Pair.Key->GetTextureSize().X * Pair.Key->GetTextureSize().Y)
		{
			TArray<float> TempData;
			TempData.AddUninitialized(Pair.Key->GetTextureSize().X * Pair.Key->GetTextureSize().Y);
			Pair.Value->PopAudio(TempData.GetData(), Pair.Key->GetTextureSize().X * Pair.Key->GetTextureSize().Y, true);
			Pair.Key->UpdateTextureSliceSyncWithFunction(reinterpret_cast<const uint8*>(TempData.GetData()), TempData.Num() * sizeof(float), 0,
				[&](FRHICommandListImmediate& RHICmdList, void* Data, const uint32 SourcePitch, const uint32 DestPitch)
				{
					float* Samples = reinterpret_cast<float*>(Data);
					for (int32 Y = 0; Y < Pair.Key->GetTextureSize().Y; Y++)
					{
						for (int32 X = 0; X < Pair.Key->GetTextureSize().X; X++)
						{
							Samples[Y * Pair.Key->GetTextureSize().X + X] = TempData[X * Pair.Key->GetTextureSize().Y + Y];
						}
					}
				});
		}
	}
}

TStatId UCompushadyAudioSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCompushadyAudioSubsystem, STATGROUP_Tickables);
}

void UCompushadyAudioSubsystem::RegisterAudioTexture(UCompushadyResource* InResource, Audio::FPatchOutputStrongPtr InPatchOutputStrongPtr)
{
	RegisteredAudioTextures.Add(InResource, InPatchOutputStrongPtr);
}