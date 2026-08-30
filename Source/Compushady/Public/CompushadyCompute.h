// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyCBV.h"
#include "CompushadySampler.h"
#include "CompushadySRV.h"
#include "CompushadyUAV.h"
#include "CompushadyTypes.h"
#include "CompushadyCompute.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyCompute : public UObject, public ICompushadyPipeline
{
	GENERATED_BODY()

public:
	bool InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages);

	bool InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages);

	bool InitFromSPIRV(const TArray<uint8>& ShaderCode, FString& ErrorMessages);

	bool InitFromDXIL(const TArray<uint8>& ShaderCode, FString& ErrorMessages);

	// Asynchronously dispatches the compute shader using ResourceArray bindings and XYZ thread group counts, invoking OnSignaled when the GPU work completes.
	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm = "ResourceArray,OnSignaled"),Category="Compushady")
	void Dispatch(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled);

	// Synchronously dispatches the compute shader, blocking until the GPU finishes, with ResourceArray bindings and XYZ thread group counts; returns false on error.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceArray"), Category = "Compushady")
	bool DispatchSync(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, FString& ErrorMessages);

	// Asynchronously dispatches the compute shader, binding resources by name via ResourceMap, with XYZ thread group counts and the OnSignaled callback.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceMap,OnSignaled"), Category = "Compushady")
	void DispatchByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FIntVector XYZ, const FCompushadySignaled& OnSignaled);

	// Synchronously dispatches the compute shader with resources bound by name via ResourceMap and XYZ thread group counts; returns false on error.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceMap"), Category = "Compushady")
	bool DispatchByMapSync(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FIntVector XYZ, FString& ErrorMessages);

	// Asynchronously dispatches the compute shader with thread group counts read from CommandBuffer at Offset (three uint32 values), invoking OnSignaled on completion.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceArray,OnSignaled"), Category = "Compushady")
	void DispatchIndirect(const FCompushadyResourceArray& ResourceArray, UCompushadyResource* CommandBuffer, const int32 Offset, const FCompushadySignaled& OnSignaled);

	// Synchronously dispatches the compute shader with thread group counts read from CommandBuffer at Offset; returns false on error.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceArray"), Category = "Compushady")
	bool DispatchIndirectSync(const FCompushadyResourceArray& ResourceArray, UCompushadyResource* CommandBuffer, const int32 Offset, FString& ErrorMessages);

	// Asynchronously dispatches the compute shader indirectly, with counts read from CommandBuffer at Offset and resources bound by name via ResourceMap.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceArray,OnSignaled"), Category = "Compushady")
	void DispatchIndirectByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, UCompushadyResource* CommandBuffer, const int32 Offset, const FCompushadySignaled& OnSignaled);

	// Synchronously dispatches the compute shader indirectly, with counts read from CommandBuffer at Offset and resources bound by name via ResourceMap; returns false on error.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceArray"), Category = "Compushady")
	bool DispatchIndirectByMapSync(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, UCompushadyResource* CommandBuffer, const int32 Offset, FString& ErrorMessages);

	// Returns true while a previously enqueued dispatch is still executing on the GPU.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	bool IsRunning() const;

	// Asynchronously dispatches the compute shader and measures its GPU execution time, reporting the elapsed duration through OnSignaledAndProfiled.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceArray,OnSignaledAndProfiled"), Category = "Compushady")
	void DispatchAndProfile(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaledAndProfiled& OnSignaledAndProfiled);

	// Asynchronously dispatches the compute shader with resources bound by name via ResourceMap and profiles its GPU execution time via OnSignaledAndProfiled.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceMap,OnSignaledAndProfiled"), Category = "Compushady")
	void DispatchByMapAndProfile(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FIntVector XYZ, const FCompushadySignaledAndProfiled& OnSignaledAndProfiled);

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FCompushadyResourceBindings ResourceBindings;

	// Returns the shader's declared thread group size (numthreads) as an XYZ vector.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetThreadGroupSize() const;

	FComputeShaderRHIRef GetRHI() const
	{
		return ComputeShaderRef;
	}

	void Dispatch_RenderThread(FRHICommandList& RHICmdList, const FCompushadyResourceArray& ResourceArray, const FIntVector& XYZ);
	void DispatchIndirect_RenderThread(FRHICommandList& RHICmdList, const FCompushadyResourceArray& ResourceArray, FBufferRHIRef BufferRHIRef, const int32 Offset);

	/* The following block is mainly used for unit testing */
	UFUNCTION()
	void StoreLastSignal(bool bSuccess, const FString& ErrorMessage);

	bool bLastSuccess = false;
	FString LastErrorMessages;

	/* end of testing block */

protected:
	FComputeShaderRHIRef ComputeShaderRef;

	FIntVector ThreadGroupSize;
};

USTRUCT(BlueprintType)
struct FCompushadyComputePass
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	UCompushadyCompute* Compute = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FCompushadyResourceArray ResourceArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector XYZ = FIntVector::ZeroValue;
};