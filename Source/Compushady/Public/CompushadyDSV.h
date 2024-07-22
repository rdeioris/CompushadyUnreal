// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyTypes.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyDSV.generated.h"

/**
 *
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyDSV : public UCompushadyResource
{
	GENERATED_BODY()

public:
	bool InitializeFromTexture(FTextureRHIRef InTextureRHIRef);

};
