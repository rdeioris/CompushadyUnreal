// Copyright 2023 - Roberto De Ioris.


#include "CompushadyDSV.h"

bool UCompushadyDSV::InitializeFromTexture(FTextureRHIRef InTextureRHIRef)
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

	RHITransitionInfo = FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::DSVWrite);

	return true;
}

void UCompushadyDSV::Clear(FLinearColor Color, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The DSV is already being processed by another task");
		return;
	}

	EnqueueToGPU([this](FRHICommandListImmediate& RHICmdList)
		{
			FRHIRenderPassInfo Info(TextureRHIRef, EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil);
			RHICmdList.BeginRenderPass(Info, TEXT("UCompushadyDSV::Clear"));
			RHICmdList.EndRenderPass();
		}, OnSignaled);
}