// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
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
	bool SetFloat(const int64 Offset, const float Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool SetDouble(const int64 Offset, const double Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool SetInt(const int64 Offset, const int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool SetUInt(const int64 Offset, const int64 Value);

	bool SetUInt(const int64 Offset, const uint32 Value);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Values"), Category = "Compushady")
	bool SetFloatArray(const int64 Offset, const TArray<float>& Values);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Values"), Category = "Compushady")
	bool SetDoubleArray(const int64 Offset, const TArray<double>& Values);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Transform"), Category = "Compushady")
	bool SetTransformFloat(const int64 Offset, const FTransform& Transform, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Transform"), Category = "Compushady")
	bool SetTransformDouble(const int64 Offset, const FTransform& Transform, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool SetPerspectiveFloat(const int64 Offset, const float HalfFOV, const int32 Width, const int32 Height, const float ZNear, const float ZFar, const bool bRightHanded = true, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool SetRotationFloat2(const int64 Offset, const float Radians);

	const TArray<uint8>& GetBufferData() const { return BufferData; }

	int64 GetBufferSize() const;

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool GetFloat(const int64 Offset, float& Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool GetDouble(const int64 Offset, double& Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool GetInt(const int64 Offset, int32& Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool GetUInt(const int64 Offset, int64& Value);

	bool GetUInt(const int64 Offset, uint32& Value);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool SetPerspectiveFromMinimalViewInfo(const int64 Offset, const FMinimalViewInfo& MinimalViewInfo, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool SetPerspectiveFromCameraComponent(const int64 Offset, UCameraComponent* CameraComponent, const bool bTranspose = true);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool SetPerspectiveFromSceneCaptureComponent2D(const int64 Offset, USceneCaptureComponent2D* SceneCaptureComponent, const bool bTranspose = true);

	bool IsValidOffset(const int64 Offset, const int64 Size) const;

	template<typename T>
	bool SetValue(const int64 Offset, const T Value)
	{
		if (IsValidOffset(Offset, sizeof(T)))
		{
			FMemory::Memcpy(BufferData.GetData() + Offset, &Value, sizeof(T));
			bBufferDataDirty = true;
			return true;
		}
		return false;
	}

	template<typename T>
	bool SetArrayValue(const int64 Offset, const TArray<T>& Values)
	{
		if (IsValidOffset(Offset, (Values.Num() * sizeof(T))))
		{
			FMemory::Memcpy(BufferData.GetData() + Offset, Values.GetData(), Values.Num() * sizeof(T));
			bBufferDataDirty = true;
			return true;
		}
		return false;
	}

	template<typename T>
	bool GetValue(const int64 Offset, T& OutValue) const
	{
		if (IsValidOffset(Offset, sizeof(T)))
		{
			const T* Value = reinterpret_cast<const T*>(BufferData.GetData() + Offset);
			OutValue = *Value;
			return true;
		}
		return false;
	}

protected:
	TArray<uint8> BufferData;
	bool bBufferDataDirty;

	FUniformBufferLayoutRHIRef UniformBufferLayoutRHIRef;
	FUniformBufferRHIRef UniformBufferRHIRef;
};
