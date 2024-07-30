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

UCLASS()
class COMPUSHADY_API UCompushadyBlitterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool AddVSPSRasterizerFromHLSL(const FString& VertexShaderSource, const FString& PixelShaderSource, const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizerConfig& RasterizerConfig, FGuid& Guid, FString& ErrorMessages, const FString& VertexShaderEntryPoint = "main", const FString& PixelShaderEntryPoint = "main");

protected:
	TSharedPtr<class FCompushadyBlitterViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
