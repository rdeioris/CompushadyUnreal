// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Compushady.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTarget2DArray.h"
#include "MediaTexture.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "CompushadyTypes.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat2 : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y = 0;

};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat3 : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Z = 0;
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat4 : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Z = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float W = 0;
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyTextureCopyInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector SourceOffset{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector SourceSize{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 SourceSlice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector DestinationOffset{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 DestinationSlice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 NumSlices = 1;
};

USTRUCT(BlueprintType)
struct FCompushadyResourceBinding
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	int32 BindingIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	int32 SlotIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	FString Name;
};

USTRUCT(BlueprintType)
struct FCompushadyResourceBindings
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TArray<FCompushadyResourceBinding> CBVs;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<FString, FCompushadyResourceBinding> CBVsMap;

	uint32 NumCBVs = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<int32, FCompushadyResourceBinding> CBVsSlotMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TArray<FCompushadyResourceBinding> SRVs;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<FString, FCompushadyResourceBinding> SRVsMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<int32, FCompushadyResourceBinding> SRVsSlotMap;

	uint32 NumSRVs = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TArray<FCompushadyResourceBinding> UAVs;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<FString, FCompushadyResourceBinding> UAVsMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<int32, FCompushadyResourceBinding> UAVsSlotMap;

	uint32 NumUAVs = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TArray<FCompushadyResourceBinding> Samplers;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<FString, FCompushadyResourceBinding> SamplersMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<int32, FCompushadyResourceBinding> SamplersSlotMap;

	uint32 NumSamplers = 0;
};

USTRUCT(BlueprintType)
struct FCompushadyResourceArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TArray<class UCompushadyCBV*> CBVs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TArray<class UCompushadySRV*> SRVs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TArray<class UCompushadyUAV*> UAVs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TArray<class UCompushadySampler*> Samplers;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCompushadySignaled, bool, bSuccess, const FString&, ErrorMessage);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCompushadySignaledWithFloatPayload, bool, bSuccess, float&, Payload, const FString&, ErrorMessage);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCompushadySignaledWithFloatArrayPayload, bool, bSuccess, const TArray<float>&, Payload, const FString&, ErrorMessage);

class COMPUSHADY_API ICompushadySignalable
{
public:
	virtual ~ICompushadySignalable() = default;

	void InitFence(UObject* InOwningObject)
	{
		OwningObject = InOwningObject;
	}

	bool IsRunning() const
	{
		return (RenderThreadCompletionEvent && !RenderThreadCompletionEvent->IsComplete()) || (GameThreadCompletionEvent && !GameThreadCompletionEvent->IsComplete());
	}

	void BeginFence(const FCompushadySignaled& OnSignaled)
	{
		RenderThreadCompletionEvent = FFunctionGraphTask::CreateAndDispatchWhenReady([] {}, TStatId(), nullptr, ENamedThreads::GetRenderThread());
		FGraphEventArray Prerequisites;
		Prerequisites.Add(RenderThreadCompletionEvent);
		GameThreadCompletionEvent = FFunctionGraphTask::CreateAndDispatchWhenReady([this, OnSignaled]
			{
				OnSignaled.ExecuteIfBound(true, "");
				OnSignalReceived();
			}, TStatId(), &Prerequisites, ENamedThreads::GameThread);
	}

	void BeginFence(const FCompushadySignaledWithFloatArrayPayload& OnSignaled, const TArray<float>& ReadbackCacheFloats)
	{
		RenderThreadCompletionEvent = FFunctionGraphTask::CreateAndDispatchWhenReady([] {}, TStatId(), nullptr, ENamedThreads::GetRenderThread());
		FGraphEventArray Prerequisites;
		Prerequisites.Add(RenderThreadCompletionEvent);
		GameThreadCompletionEvent = FFunctionGraphTask::CreateAndDispatchWhenReady([this, OnSignaled, ReadbackCacheFloats]
			{
				OnSignaled.ExecuteIfBound(true, ReadbackCacheFloats, "");
				OnSignalReceived();
			}, TStatId(), &Prerequisites, ENamedThreads::GameThread);
	}

	void WaitForGPU(FRHICommandListImmediate& RHICmdList)
	{
		RHICmdList.SubmitCommandsAndFlushGPU();
		RHICmdList.BlockUntilGPUIdle();
	}

	template<typename DELEGATE, typename... TArgs>
	void EnqueueToGPU(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction, const DELEGATE& OnSignaled, TArgs... Args)
	{
		ENQUEUE_RENDER_COMMAND(DoCompushadyEnqueueToGPU)(
			[this, InFunction](FRHICommandListImmediate& RHICmdList)
			{
				InFunction(RHICmdList);
				WaitForGPU(RHICmdList);
			});

		BeginFence(OnSignaled, Args...);
	}

	void EnqueueToGPUSync(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction)
	{
		ENQUEUE_RENDER_COMMAND(DoCompushadyEnqueueToGPU)(
			[this, InFunction](FRHICommandListImmediate& RHICmdList)
			{
				InFunction(RHICmdList);
			});

		FlushRenderingCommands();
	}

	virtual void OnSignalReceived() = 0;

protected:
	bool CopyTexture_Internal(FTextureRHIRef Destination, FTextureRHIRef Source, const FCompushadyTextureCopyInfo& CopyInfo, const FCompushadySignaled& OnSignaled);

	TWeakObjectPtr<UObject> OwningObject = nullptr;
	FGraphEventRef RenderThreadCompletionEvent = nullptr;
	FGraphEventRef GameThreadCompletionEvent = nullptr;
};

class COMPUSHADY_API ICompushadyPipeline : public ICompushadySignalable
{
public:
	virtual ~ICompushadyPipeline() = default;

	void OnSignalReceived() override;

protected:
	bool CheckResourceBindings(const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FCompushadySignaled& OnSignaled);

	void TrackResource(UObject* InResource);
	void TrackResources(const FCompushadyResourceArray& ResourceArray);
	void UntrackResources();

	// this will avoid the resources to be GC'd
	TArray<TStrongObjectPtr<UObject>> CurrentTrackedResources;
};

UCLASS(Abstract)
class COMPUSHADY_API UCompushadyResource : public UObject, public ICompushadySignalable
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackToFloatArray(const int32 Offset, const int32 Elements, const FCompushadySignaledWithFloatArrayPayload& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackAllToFloatArray(const FCompushadySignaledWithFloatArrayPayload& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackAllToFile(const FString& Filename, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackTextureToPngFile(const FString& Filename, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyToRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyFromMediaTexture(UMediaTexture* MediaTexture, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureThreadGroupSize(const FIntVector XYZ, const bool bUseNumSlicesForZ) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureSize() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int64 GetBufferSize() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetTextureNumSlices() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	EPixelFormat GetTexturePixelFormat() const;

	FTextureRHIRef GetTextureRHI() const;
	FBufferRHIRef GetBufferRHI() const;

	const FRHITransitionInfo& GetRHITransitionInfo() const;

	FStagingBufferRHIRef GetStagingBuffer();
	FBufferRHIRef GetUploadBuffer(FRHICommandListImmediate& RHICmdList);
	FTextureRHIRef GetReadbackTexture();

	bool IsValidTexture() const;
	bool IsValidBuffer() const;

	void OnSignalReceived() override;

	void MapReadAndExecute(TFunction<void(const void*)> InFunction, const FCompushadySignaled& OnSignaled);
	void MapReadAndExecuteInGameThread(TFunction<void(const void*)> InFunction, const FCompushadySignaled& OnSignaled);
	bool MapReadAndExecuteSync(TFunction<void(const void*)> InFunction);

	void MapWriteAndExecute(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled);
	void MapWriteAndExecuteInGameThread(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled);
	bool MapWriteAndExecuteSync(TFunction<void(void*)> InFunction);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "UpdateInfo", AutoCreateRefTerm = "UpdateInfo"), Category = "Compushady")
	bool UpdateTextureSliceSync(const TArray<uint8>& Pixels, const int32 Slice);

	bool UpdateTextureSliceSync(const uint8* Ptr, const int64 Size, const int32 Slice);

	bool MapTextureSliceAndExecuteSync(TFunction<void(const void*, const int32)> InFunction, const int32 Slice);

protected:
	FTextureRHIRef TextureRHIRef;
	FBufferRHIRef BufferRHIRef;
	FStagingBufferRHIRef StagingBufferRHIRef;
	FBufferRHIRef UploadBufferRHIRef;
	FRHITransitionInfo RHITransitionInfo;
	FTextureRHIRef ReadbackTextureRHIRef;
	TArray<uint8> ReadbackCacheBytes;
	TArray<float> ReadbackCacheFloats;
};

namespace Compushady
{
	namespace Utils
	{
		COMPUSHADY_API bool CreateResourceBindings(Compushady::FCompushadyShaderResourceBindings InBindings, FCompushadyResourceBindings& OutBindings, FString& ErrorMessages);
		COMPUSHADY_API bool ValidateResourceBindings(const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FPixelShaderRHIRef CreatePixelShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);

		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FPixelShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FPostProcessMaterialInputs& PPInputs);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FRayTracingShaderBindingsWriter& ShaderBindingsWriter, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings);
	}
}