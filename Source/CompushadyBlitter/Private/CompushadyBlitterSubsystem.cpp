// Copyright 2023-2025 - Roberto De Ioris.

#include "CompushadyBlitterSubsystem.h"
#include "Engine/World.h"


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
		return FGuid();
	}
	return CompushadyBlitterActor->AddDrawable(Resource, Quad, KeepAspectRatio);
}

FGuid UCompushadyBlitterSubsystem::AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return FGuid();
	}
	return CompushadyBlitterActor->AddBeforePostProcessingDrawable(Resource, Quad, KeepAspectRatio);
}

FGuid UCompushadyBlitterSubsystem::AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	ACompushadyBlitterActor* CompushadyBlitterActor = GetBlitterActor();
	if (!CompushadyBlitterActor)
	{
		return FGuid();
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
		return FGuid();
	}
	return CompushadyBlitterActor->AddViewExtension(InViewExtension, BlendableToTrack);
}
