// Copyright 2023-2025 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CompushadyRasterizer.h"
#include "CompushadyBlitterActor.generated.h"

class ICompushadyTransientBlendable;
class UCompushadyCanvas;

UCLASS()
class COMPUSHADYBLITTER_API ACompushadyBlitterActor : public AActor
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

	// Registers a Compushady resource to be blitted as a quad over the viewport after the scene is rendered; returns a handle for RemoveDrawable.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio, const FString& Label = TEXT(""));

	// Registers a Compushady resource to be blitted as a quad onto the scene color before post-processing runs; returns a handle for RemoveDrawable.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	// Registers a Compushady resource to be blitted as a quad onto the scene right after the motion blur pass; returns a handle for RemoveDrawable.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	// Registers a canvas whose items are drawn over the viewport after the scene is rendered; returns a handle for RemoveDrawable.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddCanvas(UCompushadyCanvas* Canvas);

	// Removes a previously added drawable or canvas identified by its handle from all blit stages.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void RemoveDrawable(const FGuid& Guid);

	// Returns the view matrix captured from the last rendered view, or the identity matrix if the blitter is not initialized.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const FMatrix& GetViewMatrix() const;

	// Returns the projection matrix captured from the last rendered view, or the identity matrix if the blitter is not initialized.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const FMatrix& GetProjectionMatrix() const;

	FGuid AddViewExtension(TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe> InViewExtension, TScriptInterface<IBlendableInterface> BlendableToTrack);

	// Disables and removes a tracked view extension (blendable) identified by its handle.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void RemoveViewExtension(const FGuid& Guid);

	// Removes a blendable previously added to the blitter by its handle (alias of RemoveViewExtension).
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void RemoveBlendable(const FGuid& Guid);

protected:
	TSharedPtr<class FCompushadyBlitterViewExtension, ESPMode::ThreadSafe> ViewExtension;

	TMap<FGuid, TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe>> AdditionalViewExtensions;

	UPROPERTY()
	TMap<FGuid, TScriptInterface<IBlendableInterface>> TrackedBlendables;

	// Canvases registered via AddCanvas; the strong ref keeps them alive while
	// registered, and Tick() snapshots their items to the view extension.
	UPROPERTY()
	TMap<FGuid, TObjectPtr<UCompushadyCanvas>> RegisteredCanvases;

};
