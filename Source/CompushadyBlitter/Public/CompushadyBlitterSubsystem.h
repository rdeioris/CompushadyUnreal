// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "CompushadyBlitterActor.h"
#include "Subsystems/WorldSubsystem.h"
#include "CompushadyBlitterSubsystem.generated.h"

USTRUCT()
struct FCompushadyBlitterRasterizer
{
	GENERATED_BODY();

	FGraphicsPipelineStateInitializer PipelineStateInitializer;
	FVertexShaderRHIRef VertexShaderRef;
	FCompushadyResourceBindings VSResourceBindings;

	UPROPERTY()
	FCompushadyResourceArray VSResourceArray;

	FPixelShaderRHIRef PixelShaderRef;
	FCompushadyResourceBindings PSResourceBindings;

	UPROPERTY()
	FCompushadyResourceArray PSResourceArray;

	int32 NumVertices;
	int32 NumInstances;
};

UCLASS()
class COMPUSHADYBLITTER_API UCompushadyBlitterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

	bool ShouldCreateSubsystem(UObject* Outer) const override;

	// Blits a Compushady resource as a quad over the viewport after the scene is rendered, spawning the blitter actor on demand; returns a handle for RemoveDrawable.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	// Blits a Compushady resource as a quad onto the scene color before post-processing, spawning the blitter actor on demand; returns a handle for RemoveDrawable.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	// Blits a Compushady resource as a quad onto the scene right after the motion blur pass, spawning the blitter actor on demand; returns a handle for RemoveDrawable.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	FGuid AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	// Removes a previously added drawable from the blitter actor by its handle.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void RemoveDrawable(const FGuid& Guid);

	// Returns the view matrix captured from the last rendered view by the blitter actor, or the identity matrix if unavailable.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const FMatrix& GetViewMatrix();

	// Returns the projection matrix captured from the last rendered view by the blitter actor, or the identity matrix if unavailable.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const FMatrix& GetProjectionMatrix();

	FGuid AddViewExtension(TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe> InViewExtension, TScriptInterface<IBlendableInterface> BlendableToTrack);

protected:
	ACompushadyBlitterActor* BlitterActor = nullptr;

	ACompushadyBlitterActor* GetBlitterActor();

};
