// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyCBV.h"
#include "CompushadySRV.h"
#include "CompushadyUAV.h"
#include "CompushadyCompute.generated.h"

USTRUCT(BlueprintType)
struct FCompushadyResourceBinding
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	int32 BindingIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	int32 SlotIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	FString Name;
};

USTRUCT(BlueprintType)
struct FCompushadyResourceArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UCompushadyCBV*> CBVs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UCompushadySRV*> SRVs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UCompushadyUAV*> UAVs;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCompushadySignaled, bool, bSuccess, const FString&, ErrorMessage);

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyCompute : public UObject
{
	GENERATED_BODY()

public:
	bool InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm = "ResourceArray,OnSignaled"),Category="Compushady")
	void Dispatch(const FCompushadyResourceArray& ResourceArray, const int32 X, const int32 Y, const int32 Z, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	bool IsRunning() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TMap<FString, FCompushadyResourceBinding> CBVResourceBindingsMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TMap<FString, FCompushadyResourceBinding> SRVResourceBindingsMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TMap<FString, FCompushadyResourceBinding> UAVResourceBindingsMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TMap<int32, FCompushadyResourceBinding> CBVResourceBindingsSlotMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TMap<int32, FCompushadyResourceBinding> SRVResourceBindingsSlotMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TMap<int32, FCompushadyResourceBinding> UAVResourceBindingsSlotMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TArray<FCompushadyResourceBinding> CBVResourceBindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TArray<FCompushadyResourceBinding> SRVResourceBindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TArray<FCompushadyResourceBinding> UAVResourceBindings;

protected:

	bool ToUnrealShader(const TArray<uint8>& ByteCode, TArray<uint8>& Blob, const uint32 NumCBVs, const uint32 NumSRVs, const uint32 NumUAVs);

	ERHIInterfaceType RHIInterfaceType;
	FComputeShaderRHIRef ComputeShaderRef;
	FComputePipelineStateRHIRef ComputePipelineStateRef;
	FGPUFenceRHIRef FenceRef;

	bool bRunning;

	void CheckFence(FCompushadySignaled OnSignal);

};
