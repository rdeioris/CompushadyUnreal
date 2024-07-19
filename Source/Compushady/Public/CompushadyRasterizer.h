// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyCompute.h"
#include "CompushadyDSV.h"
#include "CompushadyRTV.h"
#include "RHICommandList.h"
#include "CompushadyRasterizer.generated.h"

UENUM(BlueprintType)
enum class ECompushadyRasterizerFillMode : uint8
{
	Solid,
	Wireframe
};

UENUM(BlueprintType)
enum class ECompushadyRasterizerCullMode : uint8
{
	None,
	ClockWise,
	CounterClockWise
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyRasterizerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	ECompushadyRasterizerFillMode FillMode = ECompushadyRasterizerFillMode::Solid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	ECompushadyRasterizerCullMode CullMode = ECompushadyRasterizerCullMode::None;

};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyRasterizeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FBox Viewport;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FBox2D Scissor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 StencilValue;

};

/**
 *
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyRasterizer : public UObject, public ICompushadyPipeline
{
	GENERATED_BODY()

public:
	bool InitVSPSFromHLSL(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);
	bool InitMSPSFromHLSL(const TArray<uint8>& MeshShaderCode, const FString& MeshShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,OnSignaled"), Category = "Compushady")
	void Draw(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,OnSignaled"), Category = "Compushady")
	void DrawIndirect(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, UCompushadyResource* CommandBuffer, const int32 Offset, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void Clear(const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "MSResourceArray,PSResourceArray,OnSignaled"), Category = "Compushady")
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
	bool CreateVSPSRasterizerPipeline(TArray<uint8>& VertexShaderByteCode, TArray<uint8>& PixelShaderByteCode, Compushady::FCompushadyShaderResourceBindings VertexShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);
	bool CreateMSPSRasterizerPipeline(TArray<uint8>& MeshShaderByteCode, TArray<uint8>& PixelShaderByteCode, Compushady::FCompushadyShaderResourceBindings MeshShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);

	void FillPipelineStateInitializer(const FCompushadyRasterizerConfig& RasterizerConfig);

	ERHIInterfaceType RHIInterfaceType;
	FVertexShaderRHIRef VertexShaderRef;
	FPixelShaderRHIRef PixelShaderRef;
	FMeshShaderRHIRef MeshShaderRef;
	FGraphicsPipelineStateInitializer PipelineStateInitializer;
	FGraphicsPipelineStateRHIRef PipelineStateRHIRef;
};
