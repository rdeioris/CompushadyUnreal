// Copyright 2023-2025 - Roberto De Ioris.

#include "CompushadyBlitterSubsystem.h"

bool UCompushadyBlitterSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return (RHIGetInterfaceType() == ERHIInterfaceType::D3D12 || RHIGetInterfaceType() == ERHIInterfaceType::Vulkan);
}

void UCompushadyBlitterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{

}

void UCompushadyBlitterSubsystem::Deinitialize()
{

}

ACompushadyBlitterActor* UCompushadyBlitterSubsystem::GetBlitterActor()
{
	if (!BlitterActor)
	{
		BlitterActor = GetWorld()->SpawnActor<ACompushadyBlitterActor>();
	}
	return BlitterActor;
}

FGuid UCompushadyBlitterSubsystem::AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return FGuid::FGuid();
	}
	return CompushadyBlitterActor->AddDrawable(Resource, Quad, KeepAspectRatio);
}

FGuid UCompushadyBlitterSubsystem::AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return FGuid::FGuid();
	}
	return CompushadyBlitterActor->AddBeforePostProcessingDrawable(Resource, Quad, KeepAspectRatio);
}

FGuid UCompushadyBlitterSubsystem::AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return FGuid::FGuid();
	}
	return CompushadyBlitterActor->AddAfterMotionBlurDrawable(Resource, Quad, KeepAspectRatio);
}

void UCompushadyBlitterSubsystem::RemoveDrawable(const FGuid& Guid)
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return;
	}
	CompushadyBlitterActor->RemoveDrawable(Guid);
}

const FMatrix& UCompushadyBlitterSubsystem::GetViewMatrix()
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return FMatrix::Identity;
	}
	return CompushadyBlitterActor->GetViewMatrix();
}

const FMatrix& UCompushadyBlitterSubsystem::GetProjectionMatrix()
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return FMatrix::Identity;
	}
	return CompushadyBlitterActor->GetProjectionMatrix();
}

FGuid UCompushadyBlitterSubsystem::AddViewExtension(TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe> InViewExtension, TScriptInterface<IBlendableInterface> BlendableToTrack)
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return FGuid::FGuid();
	}
	return CompushadyBlitterActor->AddViewExtension(InViewExtension, BlendableToTrack);
}

// let's put them here to avoid circular includes
FGuid UCompushadyResource::Draw(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddDrawable(this, Quad, KeepAspectRatio);
}

FGuid UCompushadyResource::DrawBeforePostProcessing(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddBeforePostProcessingDrawable(this, Quad, KeepAspectRatio);
}

FGuid UCompushadyResource::DrawAfterMotionBlur(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddAfterMotionBlurDrawable(this, Quad, KeepAspectRatio);
}

bool UCompushadyCBV::SetProjectionMatrixFromViewport(UObject* WorldContextObject, const int64 Offset, const bool bTranspose, const bool bInverse)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FMatrix44f Matrix = FMatrix44f(WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->GetProjectionMatrix());
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

bool UCompushadyCBV::SetViewMatrixFromViewport(UObject* WorldContextObject, const int64 Offset, const bool bTranspose, const bool bInverse)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FMatrix44f Matrix = FMatrix44f(WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->GetViewMatrix());
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