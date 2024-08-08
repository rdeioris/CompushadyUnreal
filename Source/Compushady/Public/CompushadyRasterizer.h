// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CompushadyCompute.h"
#include "CompushadyDSV.h"
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
	bool InitVSPSFromHLSL(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);
	bool InitMSPSFromHLSL(const TArray<uint8>& MeshShaderCode, const FString& MeshShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);

	bool InitVSPSFromGLSL(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,RTVs,RasterizeConfig,OnSignaled"), Category = "Compushady")
	void Draw(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceMap,PSResourceMap,RTVs,RasterizeConfig,OnSignaled"), Category = "Compushady")
	void DrawByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,RTVs,RasterizeConfig,OnSignaled"), Category = "Compushady")
	void ClearAndDraw(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,RTVs,RasterizeConfig"), Category = "Compushady")
	bool ClearAndDrawSync(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, FString& ErrorMessages);


	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceMap,PSResourceMap,RTVs,RasterizeConfig,OnSignaled"), Category = "Compushady")
	void ClearAndDrawByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceMap,PSResourceMap,RTVs,RasterizeConfig"), Category = "Compushady")
	bool ClearAndDrawByMapSync(const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "VSResourceArray,PSResourceArray,RTVs,RasterizeConfig,OnSignaled"), Category = "Compushady")
	void DrawIndirect(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, UCompushadyResource* CommandBuffer, const int32 Offset, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "RTVs,OnSignaled"), Category = "Compushady")
	void Clear(const TArray<UCompushadyRTV*>& RTVs, UCompushadyDSV* DSV, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "MSResourceArray,PSResourceArray,RTVs,RasterizeConfig,OnSignaled"), Category = "Compushady")
	void DispatchMesh(const FCompushadyResourceArray& MSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const FIntVector XYZ, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled);

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
	bool CreateVSPSRasterizerPipeline(const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);
	bool CreateMSPSRasterizerPipeline(const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages);

	void FillPipelineStateInitializer(const FCompushadyRasterizerConfig& RasterizerConfig);

	bool SetupRenderTargets(const TArray<UCompushadyRTV*>& RTVs, UCompushadyDSV* DSV, TStaticArray<FRHITexture*, 8>& RenderTargets, int32& RenderTargetsEnabled, FRHITexture*& DepthStencilTexture);
	void SetupRasterization_RenderThread(FRHICommandListImmediate& RHICmdList, const FCompushadyRasterizeConfig& RasterizeConfig, const int32 Width, const int32 Height);

	static bool BeginRenderPass_RenderThread(const TCHAR* Name, FRHICommandListImmediate& RHICmdList, const TStaticArray<FRHITexture*, 8>& RenderTargets, const int32 RenderTargetsEnabled, FRHITexture* DepthStencilTexture, const ERenderTargetActions ColorAction, const EDepthStencilTargetActions DepthStencilAction, uint32& Width, uint32& Height);

	FVertexShaderRHIRef VertexShaderRef;
	FPixelShaderRHIRef PixelShaderRef;
	FMeshShaderRHIRef MeshShaderRef;
	FGraphicsPipelineStateInitializer PipelineStateInitializer;

	int32 DrawDenominator = 0;
};
