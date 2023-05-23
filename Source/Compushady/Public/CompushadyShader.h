// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataAsset.h"
#include "CompushadyShader.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyShader : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Compushady")
	FString Code;
	
};
