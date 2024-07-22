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