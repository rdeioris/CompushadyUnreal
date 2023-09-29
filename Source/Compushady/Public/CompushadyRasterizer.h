// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyCompute.h"
#include "CompushadyRTV.h"
#include "RHICommandList.h"
#include "CompushadyRasterizer.generated.h"


/**
 *
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyRasterizer : public UObject, public ICompushadyPipeline
{
	GENERATED_BODY()

public:
	bool InitVSPSFromHLSL(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, FString& ErrorMessages);
	bool InitMSPSFromHLSL(const TArray<uint8>& MeshShaderCode, const FString& MeshShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,OnSignaled"), Category = "Compushady")
	void Draw(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, const int32 NumVertices, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,OnSignaled"), Category = "Compushady")
	void DispatchMesh(const FCompushadyResourceArray& MSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, const FIntVector XYZ, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	bool IsRunning() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FCompushadyResourceBindings VSResourceBindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FCompushadyResourceBindings MSResourceBindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FCompushadyResourceBindings PSResourceBindings;

	/* The following block is mainly used for unit testing */
	UFUNCTION()
	void StoreLastSignal(bool bSuccess, const FString& ErrorMessage);

	bool bLastSuccess = false;
	FString LastErrorMessages;

	/* end of testing block */

protected:
	bool CreateVSPSRasterizerPipeline(TArray<uint8>& VertexShaderByteCode, TArray<uint8>& PixelShaderByteCode, Compushady::FCompushadyShaderResourceBindings VertexShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings, FString& ErrorMessages);
	bool CreateMSPSRasterizerPipeline(TArray<uint8>& MeshShaderByteCode, TArray<uint8>& PixelShaderByteCode, Compushady::FCompushadyShaderResourceBindings MeshShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings, FString& ErrorMessages);

	ERHIInterfaceType RHIInterfaceType;
	FVertexShaderRHIRef VertexShaderRef;
	FPixelShaderRHIRef PixelShaderRef;
	FMeshShaderRHIRef MeshShaderRef;
	FGraphicsPipelineStateInitializer PipelineStateInitializer;
};
