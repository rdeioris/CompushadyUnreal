// Copyright 2023-2024 - Roberto De Ioris.

#include "CompushadyCBV.h"
#include "Compushady.h"
#include "Kismet/GameplayStatics.h"
#if COMPUSHADY_UE_VERSION >= 54
#include "Blueprint/BlueprintExceptionInfo.h"
#endif

bool UCompushadyCBV::Initialize(const FString& Name, const uint8* Data, const int64 Size)
{
	if (Size <= 0)
	{
		return false;
	}

	// align to 16 bytes to be d3d12 compliant
	const int64 AlignedSize = Align(Size, 16);

	BufferData.AddZeroed(AlignedSize);

	if (Data)
	{
		FMemory::Memcpy(BufferData.GetData(), Data, Size);
	}

	bBufferDataDirty = true;

	FRHIUniformBufferLayoutInitializer LayoutInitializer(*Name, AlignedSize);

	UniformBufferLayoutRHIRef = RHICreateUniformBufferLayout(LayoutInitializer);
	if (!UniformBufferLayoutRHIRef.IsValid())
	{
		return false;
	}

	UniformBufferRHIRef = RHICreateUniformBuffer(nullptr, UniformBufferLayoutRHIRef, EUniformBufferUsage::UniformBuffer_MultiFrame, EUniformBufferValidation::None);
	if (!UniformBufferRHIRef.IsValid() || !UniformBufferRHIRef->IsValid())
	{
		return false;
	}

	return true;
}

FUniformBufferRHIRef UCompushadyCBV::GetRHI()
{
	return UniformBufferRHIRef;
}

bool UCompushadyCBV::BufferDataIsDirty() const
{
	return bBufferDataDirty;
}

void UCompushadyCBV::SyncBufferData(FRHICommandList& RHICmdList)
{
	RHICmdList.UpdateUniformBuffer(UniformBufferRHIRef, BufferData.GetData());
	bBufferDataDirty = false;
}

bool UCompushadyCBV::SetFloat(const int64 Offset, const float Value)
{
	return SetValue(Offset, Value);
}

bool UCompushadyCBV::SetFloatArray(const int64 Offset, const TArray<float>& Values)
{
	return SetArrayValue(Offset, Values);
}

bool UCompushadyCBV::SetInt(const int64 Offset, const int32 Value)
{
	return SetValue(Offset, Value);
}

bool UCompushadyCBV::SetUInt(const int64 Offset, const int64 Value)
{
	if (Value < 0)
	{
		return false;
	}
	return SetValue(Offset, static_cast<uint32>(Value));
}

bool UCompushadyCBV::SetUInt(const int64 Offset, const uint32 Value)
{
	return SetValue(Offset, Value);
}

bool UCompushadyCBV::SetDouble(const int64 Offset, const double Value)
{
	return SetValue(Offset, Value);
}

bool UCompushadyCBV::SetDoubleArray(const int64 Offset, const TArray<double>& Values)
{
	return SetArrayValue(Offset, Values);
}

bool UCompushadyCBV::SetIntArray(const int64 Offset, const TArray<int32>& Values)
{
	return SetArrayValue(Offset, Values);
}

bool UCompushadyCBV::SetTransformFloat(const int64 Offset, const FTransform& Transform, const bool bTranspose, const bool bInverse)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FMatrix44f Matrix(Transform.ToMatrixWithScale());
		if (bInverse)
		{
			Matrix = Matrix.Inverse();
		}
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
		return true;
	}
	return false;
}

bool UCompushadyCBV::SetTransformDouble(const int64 Offset, const FTransform& Transform, const bool bTranspose, const bool bInverse)
{
	if (IsValidOffset(Offset, 16 * sizeof(double)))
	{
		FMatrix44d Matrix(Transform.ToMatrixWithScale());
		if (bInverse)
		{
			Matrix = Matrix.Inverse();
		}
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(double));
		bBufferDataDirty = true;
		return true;
	}
	return false;
}

bool UCompushadyCBV::SetPerspectiveFloat(const int64 Offset, const float HalfFOV, const int32 Width, const int32 Height, const float ZNear, const float ZFar, const bool bRightHanded, const bool bTranspose, const bool bInverse)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FMatrix44f  Matrix = FPerspectiveMatrix44f(FMath::DegreesToRadians(HalfFOV), Width, Height, ZNear, ZFar);
		if (bRightHanded)
		{
			Matrix.M[2][2] = ((ZNear == ZFar) ? 0.0f : ZFar / (ZNear - ZFar));
			Matrix.M[2][3] *= -1;
			Matrix.M[3][2] = ((ZNear == ZFar) ? 0.0f : ZNear * ZFar / (ZNear - ZFar));
		}
		if (bInverse)
		{
			Matrix = Matrix.Inverse();
		}
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
		return true;
	}
	return false;
}

bool UCompushadyCBV::SetRotationFloat2(const int64 Offset, const float Radians)
{
	if (IsValidOffset(Offset, 4 * sizeof(float)))
	{
		float Matrix[4] = { FMath::Cos(Radians), -FMath::Sin(Radians), FMath::Sin(Radians), FMath::Cos(Radians) };
		FMemory::Memcpy(BufferData.GetData() + Offset, Matrix, 4 * sizeof(float));
		bBufferDataDirty = true;
		return true;
	}
	return false;
}

void UCompushadyCBV::BufferDataClean()
{
	bBufferDataDirty = false;
}

int64 UCompushadyCBV::GetBufferSize() const
{
	return BufferData.Num();
}

bool UCompushadyCBV::GetFloat(const int64 Offset, float& Value)
{
	return GetValue(Offset, Value);
}

bool UCompushadyCBV::GetDouble(const int64 Offset, double& Value)
{
	return GetValue(Offset, Value);
}

bool UCompushadyCBV::GetInt(const int64 Offset, int32& Value)
{
	return GetValue(Offset, Value);
}

bool UCompushadyCBV::GetUInt(const int64 Offset, int64& Value)
{
	uint32 OutValue;
	if (GetValue(Offset, OutValue))
	{
		Value = static_cast<uint32>(OutValue);
		return true;
	}
	return false;
}

bool UCompushadyCBV::GetUInt(const int64 Offset, uint32& Value)
{
	return GetValue(Offset, Value);
}

bool UCompushadyCBV::IsValidOffset(const int64 Offset, const int64 Size) const
{
	return Offset >= 0 && (Offset + Size <= BufferData.Num());
}

bool UCompushadyCBV::SetProjectionMatrixFromMinimalViewInfo(const int64 Offset, const FMinimalViewInfo& MinimalViewInfo, const bool bTranspose, const bool bInverse)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FMatrix ViewMatrix;
		FMatrix ProjectionMatrix;
		FMatrix ViewProjectionMatrix;
		UGameplayStatics::GetViewProjectionMatrix(MinimalViewInfo, ViewMatrix, ProjectionMatrix, ViewProjectionMatrix);
		FMatrix44f Matrix = FMatrix44f(ProjectionMatrix);
		if (bInverse)
		{
			Matrix = Matrix.Inverse();
		}
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
		return true;
	}
	return false;
}

bool UCompushadyCBV::SetProjectionMatrixFromPlayerCameraManager(const int64 Offset, APlayerCameraManager* PlayerCameraManager, const bool bTranspose, const bool bInverse)
{
	if (!PlayerCameraManager)
	{
		return false;
	}
	FVector OutLocation;
	FRotator OutRotation;
	PlayerCameraManager->PCOwner->GetPlayerViewPoint(OutLocation, OutRotation);
	return SetProjectionMatrixFromMinimalViewInfo(Offset, PlayerCameraManager->GetCameraCacheView(), bTranspose, bInverse);
}

bool UCompushadyCBV::SetViewMatrixFromPlayerCameraManager(const int64 Offset, APlayerCameraManager* PlayerCameraManager, const bool bTranspose, const bool bInverse)
{
	if (!PlayerCameraManager)
	{
		return false;
	}

	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		const FMinimalViewInfo& MinimalViewInfo = PlayerCameraManager->GetCameraCacheView();
		FMatrix ViewMatrix;
		FMatrix ProjectionMatrix;
		FMatrix ViewProjectionMatrix;
		UGameplayStatics::GetViewProjectionMatrix(MinimalViewInfo, ViewMatrix, ProjectionMatrix, ViewProjectionMatrix);
		FMatrix44f Matrix = FMatrix44f(ViewMatrix);
		if (bInverse)
		{
			Matrix = Matrix.Inverse();
		}
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
		return true;
	}

	return false;
}

bool UCompushadyCBV::SetProjectionMatrixFromCameraComponent(const int64 Offset, UCameraComponent* CameraComponent, const bool bTranspose, const bool bInverse)
{
	if (!CameraComponent)
	{
		return false;
	}

	FMinimalViewInfo MinimalViewInfo;
	CameraComponent->GetCameraView(0, MinimalViewInfo);
	return SetProjectionMatrixFromMinimalViewInfo(Offset, MinimalViewInfo, bTranspose, bInverse);
}

bool UCompushadyCBV::SetProjectionMatrixFromSceneCaptureComponent2D(const int64 Offset, USceneCaptureComponent2D* SceneCaptureComponent, const bool bTranspose, const bool bInverse)
{
	if (!SceneCaptureComponent)
	{
		return false;
	}

	FMinimalViewInfo MinimalViewInfo;
	SceneCaptureComponent->GetCameraView(0, MinimalViewInfo);
	return SetProjectionMatrixFromMinimalViewInfo(Offset, MinimalViewInfo, bTranspose, bInverse);
}

bool UCompushadyCBV::SetBufferData(const uint8* Data, const int32 Size)
{
	if (IsValidOffset(0, Size))
	{
		FMemory::Memcpy(BufferData.GetData(), Data, Size);
		bBufferDataDirty = true;
		return true;
	}
	return false;
}

bool UCompushadyCBV::SetScriptStruct(const int64 Offset, UScriptStruct* ScriptStruct, const uint8* Data)
{
	if (!ScriptStruct)
	{
		return false;
	}
	if (IsValidOffset(Offset, ScriptStruct->GetStructureSize()))
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, Data, ScriptStruct->GetStructureSize());
		bBufferDataDirty = true;
		return true;
	}
	return false;
}


bool UCompushadyCBV::SetStruct(const int64 Offset, const int32& Value)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UCompushadyCBV::execSetStruct)
{
	P_GET_PROPERTY(FInt64Property, Offset);

	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);

	const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	const uint8* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	if (!ValueProp || !ValuePtr)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			FText::FromString("Failed to resolve the Value for Set Struct")
		);

		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);

		P_NATIVE_BEGIN;
		*(bool*)RESULT_PARAM = false;
		P_NATIVE_END;
	}
	else
	{
		P_NATIVE_BEGIN;
		*(bool*)RESULT_PARAM = P_THIS->SetScriptStruct(Offset, ValueProp->Struct, ValuePtr);
		P_NATIVE_END;
	}
}