// Copyright 2023 - Roberto De Ioris.


#include "CompushadyCBV.h"

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
	if (!UniformBufferRHIRef.IsValid() || UniformBufferRHIRef->IsValid())
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

void UCompushadyCBV::SyncBufferData(FRHICommandListImmediate& RHICmdList)
{
	RHICmdList.UpdateUniformBuffer(UniformBufferRHIRef, BufferData.GetData());
	bBufferDataDirty = false;
}

void UCompushadyCBV::SetFloat(const int64 Offset, const float Value)
{
	SetValue(Offset, Value);
}

void UCompushadyCBV::SetFloatArray(const int64 Offset, const TArray<float>& Values)
{
	SetArrayValue(Offset, Values);
}

void UCompushadyCBV::SetInt(const int64 Offset, const int32 Value)
{
	SetValue(Offset, Value);
}

void UCompushadyCBV::SetUInt(const int64 Offset, const int64 Value)
{
	if (Value < 0)
	{
		return;
	}
	SetValue(Offset, Value);
}

void UCompushadyCBV::SetDouble(const int64 Offset, const double Value)
{
	SetValue(Offset, Value);
}

void UCompushadyCBV::SetDoubleArray(const int64 Offset, const TArray<double>& Values)
{
	SetArrayValue(Offset, Values);
}

void UCompushadyCBV::SetTransformFloat(const int64 Offset, const FTransform& Transform, const bool bTranspose)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FMatrix44f Matrix(Transform.ToMatrixWithScale());
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetTransformDouble(const int64 Offset, const FTransform& Transform, const bool bTranspose)
{
	if (IsValidOffset(Offset, 16 * sizeof(double)))
	{
		FMatrix44d Matrix(Transform.ToMatrixWithScale());
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(double));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetPerspectiveFloat(const int64 Offset, const float HalfFOV, const int32 Width, const int32 Height, const float ZNear, const float ZFar, const bool bRightHanded, const bool bTranspose)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FPerspectiveMatrix44f Matrix(FMath::DegreesToRadians(HalfFOV), Width, Height, ZNear, ZFar);
		if (bRightHanded)
		{
			Matrix.M[2][2] = ((ZNear == ZFar) ? 0.0f : ZFar / (ZNear - ZFar));
			Matrix.M[2][3] *= -1;
			Matrix.M[3][2] = ((ZNear == ZFar) ? 0.0f : ZNear * ZFar / (ZNear - ZFar));
		}
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetRotationFloat2(const int64 Offset, const float Radians)
{
	if (IsValidOffset(Offset, 4 * sizeof(float)))
	{
		float Matrix[4] = { FMath::Cos(Radians), -FMath::Sin(Radians), FMath::Sin(Radians), FMath::Cos(Radians) };
		FMemory::Memcpy(BufferData.GetData() + Offset, Matrix, 4 * sizeof(float));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::BufferDataClean()
{
	bBufferDataDirty = false;
}

int64 UCompushadyCBV::GetBufferSize() const
{
	return BufferData.Num();
}

float UCompushadyCBV::GetFloat(const int64 Offset)
{
	return GetValue<float>(Offset);
}

double UCompushadyCBV::GetDouble(const int64 Offset)
{
	return GetValue<double>(Offset);
}

int32 UCompushadyCBV::GetInt(const int64 Offset)
{
	return GetValue<int32>(Offset);
}

int64 UCompushadyCBV::GetUInt(const int64 Offset)
{
	return static_cast<int64>(GetValue<uint32>(Offset));
}

bool UCompushadyCBV::IsValidOffset(const int64 Offset, const int64 Size) const
{
	return Offset >= 0 && (Offset + Size <= BufferData.Num());
}