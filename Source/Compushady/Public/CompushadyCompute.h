// Copyright 2023 - Roberto De Ioris.

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

	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm = "ResourceArray,OnSignaled"),Category="Compushady")
	void Dispatch(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceMap,OnSignaled,SamplerMap"), Category = "Compushady")
	void DispatchByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FIntVector XYZ, const FCompushadySignaled& OnSignaled, const TMap<FString, UCompushadySampler*>& SamplerMap);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceArray,OnSignaled"), Category = "Compushady")
	void DispatchIndirect(const FCompushadyResourceArray& ResourceArray, UCompushadyResource* CommandBuffer, const int32 Offset, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	bool IsRunning() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FCompushadyResourceBindings ResourceBindings;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const TArray<uint8>& GetSPIRV() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	const TArray<uint8>& GetDXIL() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetThreadGroupSize() const;

	FComputeShaderRHIRef GetRHI() const
	{
		return ComputeShaderRef;
	}

	bool CheckResourceBindings(const FCompushadyResourceArray& ResourceArray, const FCompushadySignaled& OnSignaled);
	void Dispatch_RenderThread(FRHICommandList& RHICmdList, const FCompushadyResourceArray& ResourceArray, const FIntVector& XYZ);

	/* The following block is mainly used for unit testing */
	UFUNCTION()
	void StoreLastSignal(bool bSuccess, const FString& ErrorMessage);

	bool bLastSuccess = false;
	FString LastErrorMessages;

	/* end of testing block */

protected:

	bool CreateComputePipeline(TArray<uint8>& ByteCode, Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings, FString& ErrorMessages);

	ERHIInterfaceType RHIInterfaceType;
	FComputeShaderRHIRef ComputeShaderRef;

	FIntVector ThreadGroupSize;

	TArray<uint8> SPIRV;
	TArray<uint8> DXIL;
};
