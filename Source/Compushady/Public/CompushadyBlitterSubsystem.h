// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "CompushadyRasterizer.h"
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

class ICompushadyTransientBlendable;

UCLASS()
class COMPUSHADY_API UCompushadyBlitterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

	bool ShouldCreateSubsystem(UObject* Outer) const override;

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	FGuid AddViewExtension(TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe> InViewExtension, TScriptInterface<IBlendableInterface> BlendableToTrack);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void RemoveViewExtension(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const FMatrix& GetViewMatrix() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const FMatrix& GetProjectionMatrix() const;

protected:
	TSharedPtr<class FCompushadyBlitterViewExtension, ESPMode::ThreadSafe> ViewExtension;

	TMap<FGuid, TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe>> AdditionalViewExtensions;

	UPROPERTY()
	TMap<FGuid, TScriptInterface<IBlendableInterface>> TrackedBlendables;
};
