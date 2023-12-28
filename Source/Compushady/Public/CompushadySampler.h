// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyTypes.h"
#include "UObject/NoExportTypes.h"
#include "CompushadySampler.generated.h"

/**
 *
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadySampler : public UObject
{
	GENERATED_BODY()

public:
	bool InitializeFromSamplerState(FSamplerStateRHIRef InSamplerStateRHIRef);

	FSamplerStateRHIRef GetRHI() const;

protected:
	FSamplerStateRHIRef SamplerStateRHIRef;

};
