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
	void SetFloat(const int32 Offset, const float Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetDouble(const int32 Offset, const double Value);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Values"), Category = "Compushady")
	void SetFloatArray(const int32 Offset, const TArray<float>& Values);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Values"), Category = "Compushady")
	void SetDoubleArray(const int32 Offset, const TArray<double>& Values);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Transform"), Category = "Compushady")
	void SetTransformFloat(const int32 Offset, const FTransform& Transform, const bool bTranspose=true);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Transform"), Category = "Compushady")
	void SetTransformDouble(const int32 Offset, const FTransform& Transform, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetPerspectiveFloat(const int32 Offset, const float HalfFOV, const int32 Width, const int32 Height, const float ZNear, const float ZFar, const bool bRightHanded = true, const bool bTranspose = true);

protected:
	TArray<uint8> BufferData;
	bool bBufferDataDirty;

	FUniformBufferLayoutRHIRef UniformBufferLayoutRHIRef;
	FUniformBufferRHIRef UniformBufferRHIRef;
};
