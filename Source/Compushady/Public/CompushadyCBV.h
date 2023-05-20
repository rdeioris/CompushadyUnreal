// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyCBV.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyCBV : public UObject
{
	GENERATED_BODY()

public:
	bool Initialize(const FString& Name, const uint8* Data, const int64 Size);
	
	bool BufferDataIsDirty() const;

	void SyncBufferData(FRHICommandListImmediate& RHICmdList);

	FUniformBufferRHIRef GetRHI();

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetFloat(const int64 Offset, const float Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetDouble(const int64 Offset, const double Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetFloatArray(const int64 Offset, const TArray<float>& Values);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetDoubleArray(const int64 Offset, const TArray<double>& Values);

protected:
	TArray<uint8> BufferData;
	bool bBufferDataDirty;

	FUniformBufferLayoutRHIRef UniformBufferLayoutRHIRef;
	FUniformBufferRHIRef UniformBufferRHIRef;
};
