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

void UCompushadyCBV::SetFloat(const int64 Offset, const float Value)
{
	if (Offset + sizeof(float) < BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, &Value, sizeof(float));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetFloatArray(const int64 Offset, const TArray<float>& Values)
{
	if (Offset + (Values.Num() * sizeof(float)) < BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, Values.GetData(), Values.Num() * sizeof(float));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetDouble(const int64 Offset, const double Value)
{
	if (Offset + sizeof(double) < BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, &Value, sizeof(double));
		bBufferDataDirty = true;
	}
}

void UCompushadyCBV::SetDoubleArray(const int64 Offset, const TArray<double>& Values)
{
	if (Offset + (Values.Num() * sizeof(double)) < BufferData.Num())
	{
		FMemory::Memcpy(BufferData.GetData() + Offset, Values.GetData(), Values.Num() * sizeof(double));
		bBufferDataDirty = true;
	}
}