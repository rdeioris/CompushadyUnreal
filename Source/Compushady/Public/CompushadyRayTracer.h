// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyCompute.h"
#include "RHICommandList.h"
#include "CompushadyRayTracer.generated.h"

/**
 *
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyRayTracer : public UObject, public ICompushadyPipeline
{
	GENERATED_BODY()

public:
	bool InitFromHLSL(const TArray<uint8>& RayGenShaderCode, const FString& RayGenShaderEntryPoint, const TArray<uint8>& RayHitGroupShaderCode, const FString& RayHitGroupShaderEntryPoint, const TArray<uint8>& RayMissShaderCode, const FString& RayMissShaderEntryPoint, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ResourceArray,OnSignaled"), Category = "Compushady")
	void DispatchRays(UObject* WorldContextObject, const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	bool IsRunning() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FCompushadyResourceBindings RayGenResourceBindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FCompushadyResourceBindings RayMissResourceBindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FCompushadyResourceBindings RayHitGroupResourceBindings;

	/* The following block is mainly used for unit testing */
	UFUNCTION()
	void StoreLastSignal(bool bSuccess, const FString& ErrorMessage);

	bool bLastSuccess = false;
	FString LastErrorMessages;

	/* end of testing block */

protected:
	bool CreateRayTracerPipeline(TArray<uint8>& RayGenShaderByteCode, TArray<uint8>& RayMissShaderByteCode, TArray<uint8>& RayHitGroupShaderByteCode, Compushady::FCompushadyShaderResourceBindings RGShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings RMShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings RHGShaderResourceBindings, FString& ErrorMessages);
	
	ERHIInterfaceType RHIInterfaceType;
	FRayTracingShaderRHIRef RayGenShaderRef;
	FRayTracingShaderRHIRef RayMissShaderRef;
	FRayTracingShaderRHIRef RayHitGroupShaderRef;
	FRayTracingPipelineStateInitializer PipelineStateInitializer;
	FRayTracingPipelineState* PipelineState = nullptr;
	uint32 MaxLocalBindingDataSize = 0;
};
