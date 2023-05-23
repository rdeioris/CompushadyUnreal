// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyTypes.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat2
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y;

};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Z;
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat4
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float W;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCompushadySignaled, bool, bSuccess, const FString&, ErrorMessage);

class COMPUSHADY_API ICompushadySignalable
{
public:
	bool InitFence(UObject* InOwningObject);
	void ClearFence();
	void CheckFence(FCompushadySignaled OnSignal);
	void WriteFence(FRHICommandListImmediate& RHICmdList);
protected:
	bool bRunning;
	FGPUFenceRHIRef FenceRef;
	TWeakObjectPtr<UObject> OwningObject;
};