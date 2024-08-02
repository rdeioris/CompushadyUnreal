// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CompushadyTypes.h"
#include "AudioBusSubsystem.h"
#include "CompushadyAudioSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class COMPUSHADY_API UCompushadyAudioSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void Tick(float DeltaTime) override;
	TStatId GetStatId() const override;

	void RegisterAudioTexture(UCompushadyResource* InResource, Audio::FPatchOutputStrongPtr InPatchOutputStrongPtr);
	bool IsTickable() const override { return RegisteredAudioTextures.Num() > 0; }

	bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
	TMap<TWeakObjectPtr<UCompushadyResource>, Audio::FPatchOutputStrongPtr> RegisteredAudioTextures;
};
