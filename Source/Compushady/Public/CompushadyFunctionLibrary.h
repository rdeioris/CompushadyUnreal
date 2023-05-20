// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyCBV.h"
#include "CompushadyCompute.h"
#include "CompushadyUAV.h"
#include "Curves/CurveFloat.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CompushadyFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class COMPUSHADY_API UCompushadyFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Compushady")
    static UCompushadyCBV* CreateCompushadyCBV(const FString& Name, const int64 Size);

	UFUNCTION(BlueprintCallable, Category="Compushady")
	static UCompushadyCBV* CreateCompushadyCBVFromData(const FString& Name, const TArray<uint8>& Data);

	UFUNCTION(BlueprintCallable, CustomThunk, meta=(DisplayName = "Create Compushady CBV From Array", ArrayParm = "Data"), Category = "Compushady")
	static UCompushadyCBV* CreateCompushadyCBVFromArray(const TArray<int32>& Data);

    UFUNCTION(BlueprintCallable, Category = "Compushady")
    static UCompushadyUAV* CreateCompushadyUAVBuffer(const FString& Name, const int64 Size, const int32 Stride);

    UFUNCTION(BlueprintCallable, Category = "Compushady")
    static UCompushadyUAV* CreateCompushadyUAVTexture2D(const FString& Name, const int32 Width, const int32 Height, const EPixelFormat Format);

    UFUNCTION(BlueprintCallable, Category = "Compushady")
    static UCompushadySRV* CreateCompushadySRVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

    UFUNCTION(BlueprintCallable, Category = "Compushady")
    static UCompushadySRV* CreateCompushadySRVFromCurveFloat(UCurveFloat* CurveFloat, const float StartTime, const float EndTime, const int32 Steps);

    UFUNCTION(BlueprintCallable, Category = "Compushady")
    static UCompushadyUAV* CreateCompushadyUAVFromRenderTarget2D(UTextureRenderTarget2D* RenderTarget);

    UFUNCTION(BlueprintCallable, Category = "Compushady")
    static UCompushadyCompute* CreateCompushadyComputeFromHLSLFile(const FString& Filename, FString& ErrorMessages, const FString& EntryPoint = "main");


    DECLARE_FUNCTION(execCreateCompushadyCBVFromArray)
    {
        Stack.MostRecentProperty = nullptr;
        Stack.StepCompiledIn<FArrayProperty>(nullptr);
        FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
        if (!ArrayProperty)
        {
            Stack.bArrayContextFailed = true;
            return;
        }

        const FProperty* InnerProp = ArrayProperty->Inner;
        const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;

        uint8* ArrayAddress = Stack.MostRecentPropertyAddress;

        FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddress);
      
        UE_LOG(LogTemp, Error, TEXT("Required %d bytes (size %d) of %d [%d]"), PropertySize, InnerProp->ArrayDim, InnerProp->ElementSize, ArrayHelper.Num());

        double* Items = (double*)ArrayHelper.GetRawPtr(0);

        for (int32 i = 0; i < ArrayHelper.Num() * 12; i++)
        {
            UE_LOG(LogTemp, Error, TEXT("%d = %f"), i, Items[i]);
        } 

        P_FINISH;
        P_NATIVE_BEGIN;
        *(UCompushadyCBV**)RESULT_PARAM = NewObject<UCompushadyCBV>();
        P_NATIVE_END;
    }

	
};
