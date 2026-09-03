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

#include "GlobalRenderResources.h"
#include "TextureResource.h"

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

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<int32, FCompushadyResourceBinding> CBVsSlotMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TArray<FCompushadyResourceBinding> SRVs;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<FString, FCompushadyResourceBinding> SRVsMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<int32, FCompushadyResourceBinding> SRVsSlotMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TArray<FCompushadyResourceBinding> UAVs;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<FString, FCompushadyResourceBinding> UAVsMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<int32, FCompushadyResourceBinding> UAVsSlotMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TArray<FCompushadyResourceBinding> Samplers;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<FString, FCompushadyResourceBinding> SamplersMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Compushady")
	TMap<int32, FCompushadyResourceBinding> SamplersSlotMap;

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
	PrePostProcess,
	AfterBasePass
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

UENUM(BlueprintType)
enum class ECompushadyRasterizerBlendMode : uint8
{
	AlphaBlending,
	Always
};

UENUM(BlueprintType)
enum class ECompushadyRasterizerDepthTest : uint8
{
	AlwaysPass,
	NearOrEqual,
	FartherOrEqual
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	ECompushadyRasterizerBlendMode BlendMode = ECompushadyRasterizerBlendMode::Always;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	ECompushadyRasterizerDepthTest DepthTest = ECompushadyRasterizerDepthTest::AlwaysPass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	bool bDepthWrite = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	bool bStencilWrite = false;
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
	virtual ~ICompushadySignalable();

	bool IsRunning() const
	{
		return bRunning;
	}

	// The three BeginFence overloads now register the supplied GPU fence with the
	// FCompushadyGPUFencePoller instead of piggy-backing on an empty render-thread task.
	// The completion (bRunning = false; delegate; OnSignalReceived()) fires on the game
	// thread once the GPU has actually signalled the fence. Definitions live in
	// CompushadyTypes.cpp so this header does not need to pull in the poller.
	void BeginFence(const FCompushadySignaled& OnSignaled, const FGPUFenceRHIRef& Fence);
	void BeginFence(const FCompushadySignaledAndProfiled& OnSignaledAndProfiled, const FGPUFenceRHIRef& Fence);
	void BeginFence(const FCompushadySignaledWithFloatArrayPayload& OnSignaled, const FGPUFenceRHIRef& Fence, const TArray<float>& ReadbackCacheFloats);

	template<typename DELEGATE, typename... TArgs>
	void EnqueueToGPU(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction, const DELEGATE& OnSignaled, TArgs & ... Args)
	{
		bRunning = true;

		// Fresh, single-shot fence per dispatch (matches the engine's own readback pattern):
		// no reuse means no stale-signal race, and creation is cheap.
		FGPUFenceRHIRef SignalFence = RHICreateGPUFence(TEXT("CompushadyEnqueueToGPU"));

		ENQUEUE_RENDER_COMMAND(DoCompushadyEnqueueToGPU)(
			[this, InFunction, SignalFence](FRHICommandListImmediate& RHICmdList)
			{
				InFunction(RHICmdList);
				RHICmdList.WriteGPUFence(SignalFence.GetReference());
			});

		BeginFence(OnSignaled, SignalFence, Args...);
	}

	template<typename DELEGATE, typename... TArgs>
	void EnqueueToGPUAndProfile(TFunction<void(FRHICommandListImmediate& RHICmdList)> InFunction, const DELEGATE& OnSignaledAndProfiled, TArgs & ... Args)
	{
		bRunning = true;

		if (!QueryPoolRHIRef.IsValid())
		{
			QueryPoolRHIRef = RHICreateRenderQueryPool(ERenderQueryType::RQT_AbsoluteTime);
		}

		FGPUFenceRHIRef SignalFence = RHICreateGPUFence(TEXT("CompushadyEnqueueToGPUAndProfile"));

		ENQUEUE_RENDER_COMMAND(DoCompushadyEnqueueToGPU)(
			[this, InFunction, SignalFence](FRHICommandListImmediate& RHICmdList)
			{
				FRHIPooledRenderQuery BeforeRenderQuery = QueryPoolRHIRef->AllocateQuery();
				RHICmdList.EndRenderQuery(BeforeRenderQuery.GetQuery());

				InFunction(RHICmdList);

				FRHIPooledRenderQuery AfterRenderQuery = QueryPoolRHIRef->AllocateQuery();
				RHICmdList.EndRenderQuery(AfterRenderQuery.GetQuery());

				// Submit the recorded work, then let each query read below wait only on its own timestamp
				// (bWait = true) instead of draining the whole GPU with BlockUntilGPUIdle.
				RHICmdList.SubmitCommandsAndFlushGPU();

				uint64 BeforeMicroseconds = 0;
				RHIGetRenderQueryResult(BeforeRenderQuery.GetQuery(), BeforeMicroseconds, true);
				uint64 AfterMicroseconds = 0;
				RHIGetRenderQueryResult(AfterRenderQuery.GetQuery(), AfterMicroseconds, true);

				BeforeRenderQuery.ReleaseQuery();
				AfterRenderQuery.ReleaseQuery();

				LastQueriedMicroseconds = AfterMicroseconds - BeforeMicroseconds;

				RHICmdList.WriteGPUFence(SignalFence.GetReference());
			});

		BeginFence(OnSignaledAndProfiled, SignalFence, Args...);
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

// ---------------------------------------------------------------------------
// Blitter draw sink — registration seam for the CompushadyBlitter module.
// UCompushadyResource::Draw/DrawBeforePostProcessing/DrawAfterMotionBlur are
// UFUNCTIONs on this module's class (callable via reflection, e.g. DuPython's
// uav.draw), but the blitter that fulfils them lives in the CompushadyBlitter
// module, which already depends on this one. The blitter registers a sink at
// module startup; the Draw* methods forward through it. No sink registered ->
// warn + invalid FGuid, never a crash.
// ---------------------------------------------------------------------------
namespace Compushady::Blitter
{
	enum class EDrawPhase : uint8
	{
		Default,
		BeforePostProcessing,
		AfterMotionBlur,
	};

	using FDrawSink = TFunction<FGuid(class UCompushadyResource* Resource,
		UObject* WorldContextObject, const FVector4 Quad,
		const ECompushadyKeepAspectRatio KeepAspectRatio, const FString& Label,
		EDrawPhase Phase)>;

	COMPUSHADY_API void RegisterDrawSink(FDrawSink InSink);
	COMPUSHADY_API void UnregisterDrawSink();
}

UCLASS(Abstract, BlueprintType)
class COMPUSHADY_API UCompushadyResource : public UObject, public ICompushadyBindable, public ICompushadySignalable
{
	GENERATED_BODY()
public:

	// Asynchronously reads back 'Elements' floats starting at 'Offset' from this buffer's GPU memory, delivering them through the OnSignaled payload delegate.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackBufferToFloatArray(const int32 Offset, const int32 Elements, const FCompushadySignaledWithFloatArrayPayload& OnSignaled);

	// Synchronously reads back floats from this buffer into the Floats array (Elements <= 0 reads the whole buffer); returns false on failure.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ReadbackBufferToFloatArraySync(const int64 Offset, const int64 Elements, TArray<float>& Floats, FString& ErrorMessages);

	// Synchronously reads back raw bytes from this buffer into the Bytes array (Size <= 0 reads the whole buffer); returns false on failure.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ReadbackBufferToByteArraySync(const int64 Offset, const int64 Size, TArray<uint8>& Bytes, FString& ErrorMessages);

	// Synchronously reads back the buffer's floats as an array of FVector (3 floats each), advancing by Stride bytes per element.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<FVector> ReadbackBufferFloatsToVectorArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 12);

	// Synchronously reads back the buffer's floats as an array of FVector2D (2 floats each), advancing by Stride bytes per element.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<FVector2D> ReadbackBufferFloatsToVector2ArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 8);

	// Synchronously reads back the buffer's floats as an array of FVector4 (4 floats each), advancing by Stride bytes per element.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<FVector4> ReadbackBufferFloatsToVector4ArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 16);

	// Synchronously reads back the buffer's floats as an array of FLinearColor (RGBA, 4 floats each), advancing by Stride bytes per element.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<FLinearColor> ReadbackBufferFloatsToLinearColorArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 16);

	// Synchronously reads back the buffer's data as an array of int32 values, advancing by Stride bytes per element.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	TArray<int32> ReadbackBufferIntsToIntArraySync(const int32 Offset, const int32 Elements, const int32 Stride = 4);

	// Asynchronously reads back a range of this buffer and writes it to the given file, signaling completion via OnSignaled.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackBufferToFile(const FString& Filename, const int64 Offset, const int64 Size, const FCompushadySignaled& OnSignaled);

	// Synchronously reads back a range of this buffer and writes it to the given file (Size <= 0 writes the whole buffer); returns false on failure.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	bool ReadbackBufferToFileSync(const FString& Filename, const int64 Offset, const int64 Size, FString& ErrorMessages);



	// Asynchronously copies this resource's texture into the given Render Target 2D, signaling completion via OnSignaled.
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	// Asynchronously copies this resource's texture into the given Render Target 2D Array, signaling completion via OnSignaled.
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyToRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	// Asynchronously copies the given Media Texture into this resource's texture, signaling completion via OnSignaled.
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyFromMediaTexture(UMediaTexture* MediaTexture, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	// Asynchronously copies a region of this buffer into the destination buffer at the given offsets, signaling completion via OnSignaled.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void CopyToBuffer(UCompushadyResource* DestinationBuffer, const int64 Size, const int64 DestinationOffset, const int64 SourceOffset, const FCompushadySignaled& OnSignaled);

	// Synchronously copies a region of this buffer into the destination buffer at the given offsets; returns false on failure.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool CopyToBufferSync(UCompushadyResource* DestinationBuffer, const int64 Size, const int64 DestinationOffset, const int64 SourceOffset, FString& ErrorMessages);

	/** Returns the number of compute thread groups needed to cover this texture, dividing its dimensions by the per-group XYZ size (rounding up).
	 * When bUseNumSlicesForZ is true the Z count is derived from the texture's slice count instead of its depth. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureThreadGroupSize(const FIntVector XYZ, const bool bUseNumSlicesForZ) const;

	// Returns the number of compute thread groups needed to cover this structured buffer, dividing its element count by XYZ.X (rounding up).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetStructuredBufferThreadGroupSize(const FIntVector XYZ) const;

	// Returns this resource's texture dimensions (width, height, depth), or a zero vector if it is not a valid texture.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureSize() const;

	// Returns the size in bytes of this resource's buffer, or 0 if it is not a valid buffer.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int64 GetBufferSize() const;

	// Returns the element stride in bytes of this resource's buffer, or 0 if it is not a valid buffer.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetBufferStride() const;

	// Returns the number of array slices in this texture (multiplied by 6 for cube textures), or 0 if it is not a valid texture.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetTextureNumSlices() const;

	// Returns the pixel format of this resource's texture, or PF_Unknown if it is not a valid texture.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	EPixelFormat GetTexturePixelFormat() const;

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	FGuid Draw(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio, const FString& Label = TEXT(""));

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	FGuid DrawBeforePostProcessing(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Compushady")
	FGuid DrawAfterMotionBlur(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio);

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

	// Synchronously uploads the given pixel bytes into the specified slice of this texture; returns false while a task is running or if it is not a valid texture.
	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "UpdateInfo", AutoCreateRefTerm = "UpdateInfo"), Category = "Compushady")
	bool UpdateTextureSliceSync(const TArray<uint8>& Pixels, const int32 Slice);

	// Synchronously fills the entire buffer with the given byte value; returns false if it is not a valid buffer.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ClearBufferWithByteSync(const uint8 Value);

	// Synchronously fills the entire buffer with the given float value (per 4-byte element); returns false if it is not a valid buffer.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ClearBufferWithFloatSync(const float Value);

	// Synchronously fills the entire buffer with the given int32 value (per 4-byte element); returns false if it is not a valid buffer.
	UFUNCTION(BlueprintCallable, Category = "Compushady")
	bool ClearBufferWithIntSync(const int32 Value);

	bool UpdateTextureSliceSync(const uint8* Ptr, const int64 Size, const int32 Slice);
	bool UpdateTextureSliceSyncWithFunction(const uint8* Ptr, const int64 Size, const int32 Slice, TFunction<void(FRHICommandListImmediate& RHICmdList, void* Data, const uint32 SourcePitch, const uint32 DestPitch)> InFunction);

	bool MapTextureSliceAndExecuteSync(TFunction<void(const void*, const int32)> InFunction, const int32 Slice);

	// Async, non-stalling counterpart of MapTextureSliceAndExecuteSync: copies the slice into the
	// CPU-readback texture, then maps + runs InFunction on the game thread once the GPU fence signals.
	void MapTextureSliceAndExecute(TFunction<void(const void*, const int32)> InFunction, const int32 Slice, const FCompushadySignaled& OnSignaled);

	// The cross-process shared-handle name — the kernel-object name a peer passes to
	// OpenSharedHandleByName (D3D12); empty on Vulkan (handles are transported by value there). This
	// is distinct from the resource's RHI debug name.
	void SetSharedName(const FString& InSharedName) { SharedName = InSharedName; }

	// Cross-process sharing. The CompushadyShared module exports a native OS handle for this resource
	// and hands it over via SetSharedHandle; the resource owns it, keeps it open for its lifetime (so
	// a peer can keep opening it by name) and closes it in BeginDestroy. The platform close lives in
	// its own small translation unit (CompushadyResourceSharedHandle.cpp).
	void SetSharedHandle(void* InHandle);
	void CloseSharedHandle();

	// True if this resource was created as cross-process shared and currently owns an exported OS handle.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady|Shared")
	bool IsShared() const { return SharedHandle != nullptr; }

	// The cross-process shared name a peer opens this resource by (empty if the resource is not shared).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady|Shared")
	FString GetSharedName() const { return SharedName; }

	// Raw OS handle value (NT HANDLE / fd). Kept for the Vulkan/CUDA handle-by-value path; not needed
	// for the D3D12 name-based handshake, so it stays plain C++ rather than a Blueprint node.
	int64 GetSharedHandle() const { return reinterpret_cast<int64>(SharedHandle); }

	virtual void BeginDestroy() override;

protected:
	void* SharedHandle = nullptr;
	FString SharedName;

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
		COMPUSHADY_API bool CreateResourceBindings(const Compushady::FCompushadyShaderResourceBindings& InBindings, FCompushadyResourceBindings& OutBindings, FString& ErrorMessages);
		COMPUSHADY_API bool ValidateResourceBindings(const FCompushadyResourceArray& ResourceArray, const FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API bool ValidateResourceBindingsMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FCompushadyResourceBindings& ResourceBindings, FCompushadyResourceArray& ResourceArray, FString& ErrorMessages);

		COMPUSHADY_API FVertexShaderRHIRef CreateVertexShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FVertexShaderRHIRef CreateVertexShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FPixelShaderRHIRef CreatePixelShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FPixelShaderRHIRef CreatePixelShaderFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FString& ErrorMessages);
		COMPUSHADY_API FComputeShaderRHIRef CreateComputeShaderFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages, const FString& TargetProfile = "cs_6_0");
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

		COMPUSHADY_API void SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FComputeShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FVertexShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FMeshShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV);
		COMPUSHADY_API void SetupPipelineParametersRHI(FRHICommandList& RHICmdList, FPixelShaderRHIRef Shader, const FCompushadyResourceBindings& ResourceBindings, TFunction<FUniformBufferRHIRef(const int32)> CBVFunction, TFunction<TPair<FShaderResourceViewRHIRef, FTextureRHIRef>(const int32)> SRVFunction, TFunction<FUnorderedAccessViewRHIRef(const int32)> UAVFunction, TFunction<FSamplerStateRHIRef(const int32)> SamplerFunction, const bool bSyncCBV);

		COMPUSHADY_API void RasterizeSimplePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FVertexShaderRHIRef VertexShaderRef, FPixelShaderRHIRef PixelShaderRef, const FIntRect* ViewRect, FTextureRHIRef RenderTarget, TFunction<void()> InFunction, const FCompushadyRasterizerConfig& RasterizerConfig);
		COMPUSHADY_API void RasterizeSimplePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FVertexShaderRHIRef VertexShaderRef, FPixelShaderRHIRef PixelShaderRef, const FIntRect* ViewRect, FTextureRHIRef RenderTarget, FTextureRHIRef DepthStencil, TFunction<void()> InFunction, const FCompushadyRasterizerConfig& RasterizerConfig);
		COMPUSHADY_API void RasterizeSimplePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FVertexShaderRHIRef VertexShaderRef, FPixelShaderRHIRef PixelShaderRef, const FIntRect* ViewRect, const TArray<FTextureRHIRef>& RenderTargets, FTextureRHIRef DepthStencil, TFunction<void()> InFunction, const FCompushadyRasterizerConfig& RasterizerConfig);

		COMPUSHADY_API void RasterizePass_RenderThread(const TCHAR* PassName, FRHICommandList& RHICmdList, FGraphicsPipelineStateInitializer& PipelineStateInitializer, FTextureRHIRef RenderTarget, FTextureRHIRef DepthStencil, TFunction<void()> InFunction);

		COMPUSHADY_API bool FinalizeShader(TArray<uint8>& ByteCode, const FString& TargetProfile, Compushady::FCompushadyShaderResourceBindings& ShaderResourceBindings, FCompushadyResourceBindings& ResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages, const bool bIsSPIRV);

		COMPUSHADY_API void FillRasterizerPipelineStateInitializer(const FCompushadyRasterizerConfig& RasterizerConfig, FGraphicsPipelineStateInitializer& PipelineStateInitializer);


		COMPUSHADY_API void BytesToStringArray(const TArray<uint8>& Bytes, TArray<FString>& Lines);
		COMPUSHADY_API bool SplitToFloats(const TArray<FString>& Lines, const TArray<int32>& Columns, const FString& Separator, const int32 SkipLines, const bool bCullEmpty, TArray<float>& Values, int32& Stride);

		COMPUSHADY_API void DrawVertices(FRHICommandList& RHICmdList, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizerConfig& RasterizerConfig);
	}
}