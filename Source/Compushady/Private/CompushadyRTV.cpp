// Copyright 2023 - Roberto De Ioris.


#include "CompushadyRTV.h"

bool UCompushadyRTV::InitializeFromTexture(FTextureRHIRef InTextureRHIRef)
{
	if (!InTextureRHIRef)
	{
		return false;
	}

	TextureRHIRef = InTextureRHIRef;

	if (InTextureRHIRef->GetOwnerName() == NAME_None)
	{
		InTextureRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::RTV);

	return true;
}

void UCompushadyRTV::Clear(FLinearColor Color, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The RTV is already being processed by another task");
		return;
	}

	EnqueueToGPU([this](FRHICommandListImmediate& RHICmdList)
		{
			ClearRenderTarget(RHICmdList, GetTextureRHI(), 0, 0);
		}, OnSignaled);
}