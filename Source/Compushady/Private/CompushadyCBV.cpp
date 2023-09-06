// Copyright 2023 - Roberto De Ioris.


#include "CompushadyCBV.h"

bool UCompushadyCBV::Initialize(const FString& Name, const uint8* Data, const int64 Size)
{
	if (Size <= 0)
	{
		return false;
	}

	BufferData.AddZeroed(Size);

	if (Data)
	{
		FMemory::Memcpy(BufferData.GetData(), Data, Size);
	}

	bBufferDataDirty = true;

	FRHIUniformBufferLayoutInitializer LayoutInitializer(*Name, Size);

	UniformBufferLayoutRHIRef = RHICreateUniformBufferLayout(LayoutInitializer);
	if (!UniformBufferLayoutRHIRef.IsValid())
	{
		return false;
	}

	UniformBufferRHIRef = RHICreateUniformBuffer(nullptr, UniformBufferLayoutRHIRef, EUniformBufferUsage::UniformBuffer_MultiFrame, EUniformBufferValidation::None);
	if (!UniformBufferRHIRef.IsValid())
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

void UCompushadyCBV::SetFloat(const int32 Offset, const float Value)
{
	if (Offset + sizeof(float) <= BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, &Value, sizeof(float));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetFloatArray(const int32 Offset, const TArray<float>& Values)
{
	if (Offset + (Values.Num() * sizeof(float)) <= BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, Values.GetData(), Values.Num() * sizeof(float));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetInt(const int32 Offset, const int32 Value)
{
	if (Offset + sizeof(int32) <= BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, &Value, sizeof(int32));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetUInt(const int32 Offset, const int64 Value)
{
	const uint32 CastedValue = static_cast<uint32>(Value);
	if (Offset + sizeof(uint32) <= BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, &CastedValue, sizeof(uint32));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetDouble(const int32 Offset, const double Value)
{
	if (Offset + sizeof(double) <= BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, &Value, sizeof(double));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetDoubleArray(const int32 Offset, const TArray<double>& Values)
{
	if (Offset + (Values.Num() * sizeof(double)) <= BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, Values.GetData(), Values.Num() * sizeof(double));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetTransformFloat(const int32 Offset, const FTransform& Transform, const bool bTranspose)
{
	if (Offset + (16 * sizeof(float)) <= BufferData.Num())
	{
		FMatrix44f Matrix(Transform.ToMatrixWithScale());
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetTransformDouble(const int32 Offset, const FTransform& Transform, const bool bTranspose)
{
	if (Offset + (16 * sizeof(double)) <= BufferData.Num())
	{
		FMatrix44d Matrix(Transform.ToMatrixWithScale());
		FMemory::Memcpy(BufferData.GetData() + Offset, Matrix.M, 16 * sizeof(double));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetPerspectiveFloat(const int32 Offset, const float HalfFOV, const int32 Width, const int32 Height, const float ZNear, const float ZFar, const bool bRightHanded, const bool bTranspose)
{
	if (Offset + (16 * sizeof(float)) <= BufferData.Num())
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

void UCompushadyCBV::BufferDataClean()
{
	bBufferDataDirty = false;
}