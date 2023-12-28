// Copyright 2023 - Roberto De Ioris.


#include "CompushadySampler.h"

bool UCompushadySampler::InitializeFromSamplerState(FSamplerStateRHIRef InSamplerStateRHIRef)
{
	if (!InSamplerStateRHIRef)
	{
		return false;
	}

	SamplerStateRHIRef = InSamplerStateRHIRef;

	if (SamplerStateRHIRef->GetOwnerName() == NAME_None)
	{
		SamplerStateRHIRef->SetOwnerName(*GetPathName());
	}

	return true;
}

FSamplerStateRHIRef UCompushadySampler::GetRHI() const
{
	return SamplerStateRHIRef;
}
