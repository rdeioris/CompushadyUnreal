// Copyright 2023-2025 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CompushadyRasterizer.h"
#include "CompushadyBlitterActor.generated.h"

class ICompushadyTransientBlendable;

UCLASS()
class COMPUSHADY_API ACompushadyBlitterActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACompushadyBlitterActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void RemoveDrawable(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const FMatrix& GetViewMatrix() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const FMatrix& GetProjectionMatrix() const;

	FGuid AddViewExtension(TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe> InViewExtension, TScriptInterface<IBlendableInterface> BlendableToTrack);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void RemoveViewExtension(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void RemoveBlendable(const FGuid& Guid);

protected:
	TSharedPtr<class FCompushadyBlitterViewExtension, ESPMode::ThreadSafe> ViewExtension;

	TMap<FGuid, TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe>> AdditionalViewExtensions;

	UPROPERTY()
	TMap<FGuid, TScriptInterface<IBlendableInterface>> TrackedBlendables;

};
