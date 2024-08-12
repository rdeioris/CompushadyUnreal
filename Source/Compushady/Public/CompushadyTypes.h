// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Compushady.h"
#include "CompushadyBindable.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTarget2DArray.h"
#include "MediaTexture.h"
#include "CompushadyTypes.generated.h"

/**
 *
 */

struct FCompushadyRasterizerConfig;

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Value = 0;
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat2
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y = 0;

};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat3
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
struct COMPUSHADY_API FCompushadyFloat4
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

	TArray<Compushady::FCompushadyShaderSemantic> InputSemantics;
	TArray<Compushady::FCompushadyShaderSemantic> OutputSemantics;
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

UENUM(BlueprintType)
enum class ECompushadyKeepAspectRatio : uint8
{
	None,
	Horizontal,
	Vertical
};

UENUM(BlueprintType)
enum class ECompushadyPostProcessLocation : uint8
{
	AfterTonemapping,
	AfterMotionBlur,
	PrePostProcess
};

UENUM(BlueprintType)
enum class ECompushadySceneTexture : uint8
{
	None = 0,
	SceneColorInput,
	GBufferA,
	GBufferB,
	GBufferC,
	GBufferD,
	GBufferE,
	GBufferF,
	SceneColor,
	Depth,
	Stencil,
	CustomDepth,
	CustomStencil,
	Velocity,
	Max
};

UENUM(BlueprintType)
enum class ECompushadyShaderLanguage : uint8
{
	HLSL,
	SPIRV,
	GLSL,
	WGSL,
	MSL
};

UENUM(BlueprintType)
enum class ECompushadyWAVFormat : uint8
{
	PCM16,
	PCM32,
	PCM8,
	Float
};

struct FCompushadySceneTextures
{
	TStaticArray<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>, (uint32)ECompushadySceneTexture::Max> Textures;

	void SetTexture(const ECompushadySceneTexture SceneTexture, FTextureRHIRef Texture)
	{
		if (!Texture)
		{
			Texture = GTransparentBlackTexture->GetTextureRHI();
		}
		Textures[(uint32)SceneTexture] = { nullptr, Texture };
	}

	void SetSRV(const ECompushadySceneTexture SceneTexture, FShaderResourceViewRHIRef SRV)
	{
		if (!SRV)
		{
			SRV = GTransparentBlackTextureWithSRV->ShaderResourceViewRHI;
		}
		Textures[(uint32)SceneTexture] = { SRV, nullptr };
	}
};

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

UENUM(BlueprintType)
enum class ECompushadyRasterizerPrimitiveType : uint8
{
	TriangleList,
	LineList,
	PointList
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyRasterizerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	ECompushadyRasterizerFillMode FillMode = ECompushadyRasterizerFillMode::Solid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	ECompushadyRasterizerCullMode CullMode = ECompushadyRasterizerCullMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	ECompushadyRasterizerPrimitiveType PrimitiveType = ECompushadyRasterizerPrimitiveType::TriangleList;
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

	FCompushadyRasterizeConfig()
	{
		Viewport.Init();
		Scissor.Init();
		StencilValue = 0;
	}

};

UENUM()
enum ECompushadySamplerAddressMode : uint8
{
	Wrap,
	Clamp,
	Mirror,
	Border
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCompushadySignaled, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCompushadySignaledAndProfiled, bool, bSuccess, const int64, Microseconds, const FString&, ErrorMessage);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCompushadySignaledWithFloatPayload, bool, bSuccess, float&, Payload, const FString&, ErrorMessage);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCompushadySignaledWithFloatArrayPayload, bool, bSuccess, const TArray<float>&, Payload, const FString&, ErrorMessage);

class COMPUSHADY_API ICompushadySignalable
{
public:
	virtual ~ICompushadySignalable() = default;

	bool IsRunning() const
	{
		return bRunning;
	}

	void BeginFence(const FCompushadySignaled& OnSignaled)
	{
		FGraphEventRef RenderThreadCompletionEvent = FFunctionGraphTask::CreateAndDispatchWhenReady([] {}, TStatId(), nullptr, ENamedThreads::GetRenderThread());
		FGraphEventArray Prerequisites = { RenderThreadCompletionEvent };
		FFunctionGraphTask::CreateAndDispatchWhenReady([this, OnSignaled]
			{
				bRunning = false;
				OnSignaled.ExecuteIfBound(true, "");
				OnSignalReceived();
			}, TStatId(), &Prerequisites, ENamedThreads::GameThread);
	}

	void BeginFence(const FCompushadySignaledAndProfiled& OnSignaledAndProfiled)
	{
		FGraphEventRef RenderThreadCompletionEvent = FFunctionGraphTask::CreateAndDispatchWhenReady([] {}, TStatId(), nullptr, ENamedThreads::GetRenderThread());
		FGraphEventArray Prerequisites = { RenderThreadCompletionEvent };
		FFunctionGraphTask::CreateAndDispatchWhenReady([this, OnSignaledAndProfiled]
			{
				bRunning = false;
				OnSignaledAndProfiled.ExecuteIfBound(true, LastQueriedMicroseconds, "");
				OnSignalReceived();
			}, TStatId(), &Prerequisites, ENamedThreads::GameThread);
	}

	void BeginFence(const FCompushadySignaledWithFloatArrayPayload& OnSignaled, const TArray<float>& ReadbackCacheFloats)
	{
		FGraphEventRef RenderThreadCompletionEvent = FFunctionGraphTask::CreateAndDispatchWhenReady([] {}, TStatId(), nullptr, ENamedThreads::GetRenderThread());
		FGraphEventArray Prerequisites = { RenderThreadCompletionEvent };
		FFunctionGraphTask::CreateAndDispatchWhenReady([this, OnSignaled, &ReadbackCacheFloats]
			{
				bRunning = false;
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
	void EnqueueToGPU(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction, const DELEGATE& OnSignaled, TArgs & ... Args)
	{
		bRunning = true;
		ENQUEUE_RENDER_COMMAND(DoCompushadyEnqueueToGPU)(
			[this, InFunction](FRHICommandListImmediate& RHICmdList)
			{
				InFunction(RHICmdList);
			});

		BeginFence(OnSignaled, Args...);
	}

	template<typename DELEGATE, typename... TArgs>
	void EnqueueToGPUAndProfile(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction, const DELEGATE& OnSignaledAndProfiled, TArgs & ... Args)
	{
		bRunning = true;

		if (!QueryPoolRHIRef.IsValid())
		{
			QueryPoolRHIRef = RHICreateRenderQueryPool(ERenderQueryType::RQT_AbsoluteTime);
		}

		ENQUEUE_RENDER_COMMAND(DoCompushadyEnqueueToGPU)(
			[this, InFunction](FRHICommandListImmediate& RHICmdList)
			{
				FRHIPooledRenderQuery BeforeRenderQuery = QueryPoolRHIRef->AllocateQuery();
				RHICmdList.EndRenderQuery(BeforeRenderQuery.GetQuery());

				InFunction(RHICmdList);

				FRHIPooledRenderQuery AfterRenderQuery = QueryPoolRHIRef->AllocateQuery();
				RHICmdList.EndRenderQuery(AfterRenderQuery.GetQuery());

				WaitForGPU(RHICmdList);

				uint64 BeforeMicroseconds = 0;
				RHIGetRenderQueryResult(BeforeRenderQuery.GetQuery(), BeforeMicroseconds, false);
				uint64 AfterMicroseconds = 0;
				RHIGetRenderQueryResult(AfterRenderQuery.GetQuery(), AfterMicroseconds, false);

				BeforeRenderQuery.ReleaseQuery();
				AfterRenderQuery.ReleaseQuery();

				LastQueriedMicroseconds = AfterMicroseconds - BeforeMicroseconds;
			});

		BeginFence(OnSignaledAndProfiled, Args...);
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

	bool bRunning = false;

	FRenderQueryPoolRHIRef QueryPoolRHIRef;
	int64 LastQueriedMicroseconds = 0;
};

class COMPUSHADY_API ICompushadyPipeline : public ICompushadySignalable
{
public:
	virtual ~ICompushadyPipeline() = default;

	void OnSignalReceived() override;

	void TrackResourcesAndMarkAsRunning(const FCompushadyResourceArray& ResourceArray);
	void UntrackResourcesAndUnmarkAsRunning();

protected:

	void TrackResource(UObject* InResource);
	void TrackResources(const FCompushadyResourceArray& ResourceArray);
	void UntrackResources();

	// this will avoid the resources to be GC'd
	TArray<TStrongObjectPtr<UObject>> CurrentTrackedResources;
};

UCLASS(Abstract, BlueprintType)
class COMPUSHADY_API UCompushadyResource : public UObject, public ICompushadyBindable, public ICompushadySignalable
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackBufferToFloatArray(const int32 Offset, const int32 Elements, const FCompushadySignaledWithFloatArrayPayload& OnSignaled);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ReadbackBufferToFloatArraySync(const int64 Offset, const int64 Elements, TArray<float>& Floats, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ReadbackBufferToByteArraySync(const int64 Offset, const int64 Size, TArray<uint8>& Bytes, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<FVector> ReadbackBufferFloatsToVectorArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 12);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<FVector2D> ReadbackBufferFloatsToVector2ArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 8);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<FVector4> ReadbackBufferFloatsToVector4ArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 16);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<FLinearColor> ReadbackBufferFloatsToLinearColorArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 16);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<int32> ReadbackBufferIntsToIntArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 4);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackBufferToFile(const FString& Filename, const int64 Offset, const int64 Size, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	bool ReadbackBufferToFileSync(const FString& Filename, const int64 Offset, const int64 Size, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ReadbackTextureToPNGFileSync(const FString& Filename, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ReadbackTextureToTIFFFileSync(const FString& Filename, const FString& ImageDescription, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ReadbackBufferToWAVFileSync(const FString& Filename, const int64 Offset, const int64 Size, const int32 SampleRate, const int32 NumChannels, const EPixelFormat PixelFormat, const ECompushadyWAVFormat WAVFormat, FString& ErrorMessages);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyToRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyFromMediaTexture(UMediaTexture* MediaTexture, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureThreadGroupSize(const FIntVector XYZ, const bool bUseNumSlicesForZ) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetStructuredBufferThreadGroupSize(const FIntVector XYZ) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureSize() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int64 GetBufferSize() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetTextureNumSlices() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	EPixelFormat GetTexturePixelFormat() const;

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	void Draw(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	void DrawBeforePostProcessing(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	void DrawAfterMotionBlur(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

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
	bool MapReadAndExecuteSync(TFunction<bool(const void*)> InFunction);

	void MapWriteAndExecute(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled);
	void MapWriteAndExecuteInGameThread(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled);
	bool MapWriteAndExecuteSync(TFunction<bool(void*)> InFunction);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "UpdateInfo", AutoCreateRefTerm = "UpdateInfo"), Category = "Compushady")
	bool UpdateTextureSliceSync(const TArray<uint8>& Pixels, const int32 Slice);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ClearBufferWithValueSync(const uint8 Value);

	bool UpdateTextureSliceSync(const uint8* Ptr, const int64 Size, const int32 Slice);
	bool UpdateTextureSliceSyncWithFunction(const uint8* Ptr, const int64 Size, const int32 Slice, TFunction<void(FRHICommandListImmediate& RHICmdList, void* Data, const uint32 SourcePitch, const uint32 DestPitch)> InFunction);

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
		COMPUSHADY_API bool ValidateResourceBindingsMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FCompushadyResourceBindings& ResourceBindings, FCompushadyResourceArray& ResourceArray, FString& ErrorMessages);

		COMPUSHADY_API FVertexShaderRHIRef CreateVertexShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FVertexShaderRHIRef CreateVertexShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FPixelShaderRHIRef CreatePixelShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FPixelShaderRHIRef CreatePixelShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FComputeShaderRHIRef CreateComputeShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
		COMPUSHADY_API FComputeShaderRHIRef CreateComputeShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
		COMPUSHADY_API FComputeShaderRHIRef CreateComputeShaderFromSPIRVBlob(const TArray<uint8>& ShaderByteCode, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
		COMPUSHADY_API FMeshShaderRHIRef CreateMeshShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
		COMPUSHADY_API FMeshShaderRHIRef CreateMeshShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);

		COMPUSHADY_API FVertexShaderRHIRef CreateVertexShaderFromHLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FVertexShaderRHIRef CreateVertexShaderFromGLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FPixelShaderRHIRef CreatePixelShaderFromHLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FPixelShaderRHIRef CreatePixelShaderFromGLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FComputeShaderRHIRef CreateComputeShaderFromHLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
		COMPUSHADY_API FComputeShaderRHIRef CreateComputeShaderFromGLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
		COMPUSHADY_API FMeshShaderRHIRef CreateMeshShaderFromHLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
		COMPUSHADY_API FMeshShaderRHIRef CreateMeshShaderFromGLSL(const FString& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);

		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FCompushadySceneTextures& SceneTextures, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FPixelShaderRHIRef Shader, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const FCompushadySceneTextures& SceneTextures, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParameters(FRHICommandList& RHICmdList, FRayTracingShaderBindingsWriter& ShaderBindingsWriter, const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, const bool bSyncCBV);

		COMPUSHADY_API void SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FPixelShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV);

		COMPUSHADY_API void RasterizeSimplePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FVertexShaderRHIRef VertexShaderRef, FPixelShaderRHIRef PixelShaderRef, FTextureRHIRef RenderTarget, TFunction<void()> InFunction, const FCompushadyRasterizerConfig& RasterizerConfig);
		COMPUSHADY_API void RasterizeSimplePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FVertexShaderRHIRef VertexShaderRef, FPixelShaderRHIRef PixelShaderRef, FTextureRHIRef RenderTarget, FTextureRHIRef DepthStencil, TFunction<void()> InFunction, const FCompushadyRasterizerConfig& RasterizerConfig);

		COMPUSHADY_API void RasterizePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FGraphicsPipelineStateInitializer& PipelineStateInitializer, FTextureRHIRef RenderTarget, FTextureRHIRef DepthStencil, TFunction<void()> InFunction);

		COMPUSHADY_API bool FinalizeShader(TArray<uint8>& ByteCode, const FString& TargetProfile, Compushady::FCompushadyShaderResourceBindings& ShaderResourceBindings, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages, const bool bIsSPIRV);

		COMPUSHADY_API void FillRasterizerPipelineStateInitializer(const FCompushadyRasterizerConfig& RasterizerConfig, FGraphicsPipelineStateInitializer& PipelineStateInitializer);

		COMPUSHADY_API bool GenerateTIFF(const void* Data, const int32 Stride, const uint32 Width, const uint32 Height, const EPixelFormat PixelFormat, const FString ImageDescription, TArray<uint8>& IFD);
		COMPUSHADY_API bool LoadNRRD(const FString& Filename, TArray64<uint8>& SlicesData, int64& Offset, uint32& Width, uint32& Height, uint32& Depth, EPixelFormat& PixelFormat);

		COMPUSHADY_API void DrawVertices(FRHICommandList& RHICmdList, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizerConfig& RasterizerConfig);
	}
}