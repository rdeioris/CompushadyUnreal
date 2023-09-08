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

	void BufferDataClean();

	FUniformBufferRHIRef GetRHI();

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetFloat(const int64 Offset, const float Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetDouble(const int64 Offset, const double Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetInt(const int64 Offset, const int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetUInt(const int64 Offset, const int64 Value);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Values"), Category = "Compushady")
	void SetFloatArray(const int64 Offset, const TArray<float>& Values);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Values"), Category = "Compushady")
	void SetDoubleArray(const int64 Offset, const TArray<double>& Values);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Transform"), Category = "Compushady")
	void SetTransformFloat(const int64 Offset, const FTransform& Transform, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Transform"), Category = "Compushady")
	void SetTransformDouble(const int64 Offset, const FTransform& Transform, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetPerspectiveFloat(const int64 Offset, const float HalfFOV, const int32 Width, const int32 Height, const float ZNear, const float ZFar, const bool bRightHanded = true, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetRotationFloat2(const int64 Offset, const float Radians);

	const TArray<uint8>& GetBufferData() const { return BufferData; }

	int64 GetBufferSize() const;

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	float GetFloat(const int64 Offset);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	double GetDouble(const int64 Offset);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	int32 GetInt(const int64 Offset);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	int64 GetUInt(const int64 Offset);

	bool IsValidOffset(const int64 Offset, const int64 Size) const;

	template<typename T>
	void SetValue(const int64 Offset, const T Value)
	{
		if (IsValidOffset(Offset, sizeof(T)))
		{
			FMemory::Memcpy(BufferData.GetData() + Offset, &Value, sizeof(T));
			bBufferDataDirty = true;
		}
	}

	template<typename T>
	void SetArrayValue(const int64 Offset, const TArray<T>& Values)
	{
		if (IsValidOffset(Offset, (Values.Num() * sizeof(T))))
		{
			FMemory::Memcpy(BufferData.GetData() + Offset, Values.GetData(), Values.Num() * sizeof(T));
			bBufferDataDirty = true;
		}
	}

	template<typename T>
	T GetValue(const int64 Offset) const
	{
		if (IsValidOffset(Offset, sizeof(T)))
		{
			const T* Value = reinterpret_cast<const T*>(BufferData.GetData() + Offset);
			return *Value;
		}
		return 0;
	}

protected:
	TArray<uint8> BufferData;
	bool bBufferDataDirty;

	FUniformBufferLayoutRHIRef UniformBufferLayoutRHIRef;
	FUniformBufferRHIRef UniformBufferRHIRef;
};
