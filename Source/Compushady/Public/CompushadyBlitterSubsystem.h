// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "CompushadyTypes.h"
#include "CompushadyBlitterSubsystem.generated.h"

UCLASS()
class COMPUSHADY_API UCompushadyBlitterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:	
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Compushady")
	void AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

protected:
	TSharedPtr<class FCompushadyBlitterViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
