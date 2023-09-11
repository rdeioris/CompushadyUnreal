// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyCBV.h"
#include "CompushadySRV.h"
#include "CompushadyUAV.h"
#include "CompushadyTypes.h"
#include "CompushadyCompute.generated.h"

USTRUCT(BlueprintType)
struct FCompushadyResourceBinding
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	int32 BindingIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	int32 SlotIndex = 0;

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

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyCompute : public UObject, public ICompushadySignalable
{
	GENERATED_BODY()

public:
	bool InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages);

	bool InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages);

	bool InitFromSPIRV(const TArray<uint8>& ShaderCode, FString& ErrorMessages);

	bool InitFromDXIL(const TArray<uint8>& ShaderCode, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm = "ResourceArray,OnSignaled"),Category="Compushady")
	void Dispatch(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceMap,OnSignaled"), Category = "Compushady")
	void DispatchByMap(const TMap<FString, UCompushadyResource*>& ResourceMap, const FIntVector XYZ, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ResourceArray,OnSignaled"), Category = "Compushady")
	void DispatchIndirect(const FCompushadyResourceArray& ResourceArray, UCompushadyResource* Buffer, const int32 Offset, const FCompushadySignaled& OnSignaled);

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

	void OnSignalReceived() override;

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

	/* The following block is mainly used for unit testing */
	UFUNCTION()
	void StoreLastSignal(bool bSuccess, const FString& ErrorMessage);

	bool bLastSuccess = false;
	FString LastErrorMessages;

	/* end of testign block */

protected:

	bool CreateComputePipeline(TArray<uint8>& ByteCode, Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings, FString& ErrorMessages);

	bool ToUnrealShader(const TArray<uint8>& ByteCode, TArray<uint8>& Blob, const uint32 NumCBVs, const uint32 NumSRVs, const uint32 NumUAVs);

	ERHIInterfaceType RHIInterfaceType;
	FComputeShaderRHIRef ComputeShaderRef;
	FComputePipelineStateRHIRef ComputePipelineStateRef;

	// this will avoid the resources to be GC'd
	UPROPERTY()
	FCompushadyResourceArray CurrentResourceArray;

	FIntVector ThreadGroupSize;

	TArray<uint8> SPIRV;
	TArray<uint8> DXIL;

	bool SetupDispatch(const FCompushadyResourceArray& ResourceArray, const FCompushadySignaled& OnSignaled);
	void SetupPipeline(FRHICommandListImmediate& RHICmdList, const TArray<UCompushadyCBV*>& CBVs, const TArray<UCompushadySRV*>& SRVs, const TArray<UCompushadyUAV*>& UAVs);

};
