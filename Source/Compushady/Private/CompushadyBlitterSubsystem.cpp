// Copyright 2023-2024 - Roberto De Ioris.

#include "CompushadyBlitterSubsystem.h"
#if COMPUSHADY_UE_VERSION >= 53
#include "PostProcess/PostProcessMaterialInputs.h"
#include "ScreenPass.h"
#endif
#include "RenderGraphBuilder.h"
#include "SceneViewExtension.h"
#include "FXRenderingUtils.h"
#include "CompushadyBlendable.h"

struct FCompushadyBlitterDrawable
{
	FGuid Guid;
	FTextureRHIRef Texture;
	FShaderResourceViewRHIRef SRV;
	FVector4 Quad;
	ECompushadyKeepAspectRatio AspectRatio;
};

class FCompushadyBlitterViewExtension : public FSceneViewExtensionBase
{
protected:
	FIntRect GetScreenSize(const FSceneView& View, const bool bBeforeUpscaling)
	{
		FIntRect CurrentViewRect = View.UnconstrainedViewRect;
		if (bBeforeUpscaling && View.bIsViewInfo)
		{
			CurrentViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
		}
		return CurrentViewRect;
	};
public:
	FCompushadyBlitterViewExtension(const FAutoRegister& AutoRegister, FVertexShaderRHIRef InVertexShaderRef, const FCompushadyResourceBindings& InVSResourceBindings, FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings) :
		FSceneViewExtensionBase(AutoRegister),
		VertexShaderRef(InVertexShaderRef),
		VSResourceBindings(InVSResourceBindings),
		PixelShaderRef(InPixelShaderRef),
		PSResourceBindings(InPSResourceBindings)
	{
		// ensure 16 bytes alignment!
		FRHIUniformBufferLayoutInitializer LayoutInitializer(nullptr, 96);
		UniformBufferLayoutRef = RHICreateUniformBufferLayout(LayoutInitializer);
		UniformBufferRef = RHICreateUniformBuffer(nullptr, UniformBufferLayoutRef, EUniformBufferUsage::UniformBuffer_MultiFrame, EUniformBufferValidation::None);

		FSamplerStateInitializerRHI SamplerStateInitializer(ESamplerFilter::SF_Bilinear, ESamplerAddressMode::AM_Clamp, ESamplerAddressMode::AM_Clamp, ESamplerAddressMode::AM_Clamp);
		SamplerStateRef = RHICreateSamplerState(SamplerStateInitializer);
	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override
	{
		CurrentViewMatrix = InView.ViewMatrices.GetViewMatrix();
		CurrentProjectionMatrix = InView.ViewMatrices.GetProjectionMatrix();
	}

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	void DrawDrawables_RenderThread(FRHICommandList& RHICmdList, const FIntRect ViewRect, const TArray<FCompushadyBlitterDrawable>& CurrentDrawables)
	{
		TStaticArray<float, 24> QuadData;

		for (const FCompushadyBlitterDrawable& Drawable : CurrentDrawables)
		{
			FVector4 Quad = Drawable.Quad;

			if (Drawable.AspectRatio != ECompushadyKeepAspectRatio::None)
			{
				const float ScreenWidth = static_cast<float>(ViewRect.Width());
				const float ScreenHeight = static_cast<float>(ViewRect.Height());
				const float ImageWidth = static_cast<float>(Drawable.Texture->GetSizeX());
				const float ImageHeight = static_cast<float>(Drawable.Texture->GetSizeY());

				if (Drawable.AspectRatio == ECompushadyKeepAspectRatio::Horizontal)
				{
					const float WantedImageWidth = ScreenWidth * (Quad.Z - Quad.X);
					const float NewImageHeight = ImageHeight * (WantedImageWidth / ImageWidth);
					Quad.W = Quad.Y + (NewImageHeight / ScreenHeight);
				}
				else if (Drawable.AspectRatio == ECompushadyKeepAspectRatio::Vertical)
				{
					const float WantedImageHeight = ScreenHeight * (Quad.W - Quad.Y);
					const float NewImageWidth = ImageWidth * (WantedImageHeight / ImageHeight);
					Quad.Z = Quad.X + (NewImageWidth / ScreenWidth);
				}
			}

			// top left
			QuadData[0] = Quad.X * 2 - 1;
			QuadData[1] = -(Quad.Y * 2 - 1);
			QuadData[2] = 0;
			QuadData[3] = 1;

			// top right
			QuadData[4] = Quad.Z * 2 - 1;
			QuadData[5] = -(Quad.Y * 2 - 1);
			QuadData[6] = 0;
			QuadData[7] = 1;

			// bottom left
			QuadData[8] = Quad.X * 2 - 1;
			QuadData[9] = -(Quad.W * 2 - 1);
			QuadData[10] = 0;
			QuadData[11] = 1;

			// top right
			QuadData[12] = Quad.Z * 2 - 1;
			QuadData[13] = -(Quad.Y * 2 - 1);
			QuadData[14] = 0;
			QuadData[15] = 1;

			// bottom right
			QuadData[16] = Quad.Z * 2 - 1;
			QuadData[17] = -(Quad.W * 2 - 1);
			QuadData[18] = 0;
			QuadData[19] = 1;

			// bottom left
			QuadData[20] = Quad.X * 2 - 1;
			QuadData[21] = -(Quad.W * 2 - 1);
			QuadData[22] = 0;
			QuadData[23] = 1;

			RHICmdList.UpdateUniformBuffer(UniformBufferRef, QuadData.GetData());

			Compushady::Utils::SetupPipelineParametersRHI(RHICmdList, VertexShaderRef, VSResourceBindings,
				[&](const int32 Index)
				{
					return UniformBufferRef;
				},
				[&](const int32 Index) -> TPair<FShaderResourceViewRHIRef, FTextureRHIRef>
				{
					RHICmdList.Transition(FRHITransitionInfo(Drawable.Texture, ERHIAccess::Unknown, ERHIAccess::SRVMask));
					return { Drawable.SRV, Drawable.SRV ? nullptr : Drawable.Texture };
				},
				[](const int32 Index)
				{
					return nullptr;
				},
				[&](const int32 Index)
				{
					return SamplerStateRef;
				},
				true);

			Compushady::Utils::SetupPipelineParametersRHI(RHICmdList, PixelShaderRef, PSResourceBindings,
				[&](const int32 Index)
				{
					return UniformBufferRef;
				},
				[&](const int32 Index) -> TPair<FShaderResourceViewRHIRef, FTextureRHIRef>
				{
					RHICmdList.Transition(FRHITransitionInfo(Drawable.Texture, ERHIAccess::Unknown, ERHIAccess::SRVMask));
					return { Drawable.SRV, Drawable.SRV ? nullptr : Drawable.Texture };
				},
				[](const int32 Index)
				{
					return nullptr;
				},
				[&](const int32 Index)
				{
					return SamplerStateRef;
				},
				true);

			RHICmdList.DrawPrimitive(0, 2, 1);
		}
	}

	void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override
	{
		if (InView.bIsSceneCapture || InView.bIsSceneCaptureCube || InView.bIsReflectionCapture || InView.bIsPlanarReflection)
		{
			return;
		}

		TArray<FCompushadyBlitterDrawable> CurrentDrawables;
		{
			FScopeLock DrawablesLock(&DrawablesCriticalSection);
			CurrentDrawables = Drawables;
		}

		if (CurrentDrawables.Num() > 0)
		{
			FTexture2DRHIRef RenderTarget = InView.Family->RenderTarget->GetRenderTargetTexture();

			if (!InView.Family->RenderTarget->GetRenderTargetTexture().IsValid())
			{
				return;
			}

			const FIntRect ViewRect = GetScreenSize(InView, false);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("FCompushadyDrawerViewExtension::PostRenderView_RenderThread"),
				ERDGPassFlags::None,
				[this, ViewRect, RenderTarget, CurrentDrawables](FRHICommandList& RHICmdList)
				{
					FCompushadyRasterizerConfig RasterizerConfig;
					RasterizerConfig.BlendMode = ECompushadyRasterizerBlendMode::AlphaBlending;

					Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyDrawerViewExtension::PostRenderView_RenderThread"),
						RHICmdList, VertexShaderRef, PixelShaderRef, &ViewRect, RenderTarget, [&]()
						{
							DrawDrawables_RenderThread(RHICmdList, ViewRect, CurrentDrawables);
						}, RasterizerConfig);
				});
		}
	}

	void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override
	{
		if (View.bIsSceneCapture || View.bIsSceneCaptureCube || View.bIsReflectionCapture || View.bIsPlanarReflection)
		{
			return;
		}

		TArray<FCompushadyBlitterDrawable> CurrentDrawables;
		{
			FScopeLock DrawablesLock(&BeforePostProcessingDrawablesCriticalSection);
			CurrentDrawables = BeforePostProcessingDrawables;
		}

		if (CurrentDrawables.Num() > 0)
		{
			TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures = *((TRDGUniformBufferRef<FSceneTextureUniformParameters>*) & Inputs);

			const FIntRect ViewRect = GetScreenSize(View, true);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("FCompushadyDrawerViewExtension::PrePostProcessPass_RenderThread"),
				ERDGPassFlags::None,
				[this, ViewRect, SceneTextures, CurrentDrawables](FRHICommandList& RHICmdList)
				{
					FTexture2DRHIRef RenderTarget = SceneTextures->GetContents()->SceneColorTexture->GetRHI();
					FCompushadyRasterizerConfig RasterizerConfig;
					RasterizerConfig.BlendMode = ECompushadyRasterizerBlendMode::AlphaBlending;
					Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyDrawerViewExtension::PrePostProcessPass_RenderThread"),
						RHICmdList, VertexShaderRef, PixelShaderRef, &ViewRect, RenderTarget, [&]()
						{
							DrawDrawables_RenderThread(RHICmdList, ViewRect, CurrentDrawables);
						}, RasterizerConfig);
				});
		}
	}

#if COMPUSHADY_UE_VERSION >= 53
	FScreenPassTexture PostProcessAfterMotionBlur_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
	{
#if COMPUSHADY_UE_VERSION >= 54
		const FScreenPassTexture& SceneColorInput = FScreenPassTexture::CopyFromSlice(GraphBuilder, InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor));
#else
		FScreenPassTexture SceneColorInput = InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor);
#endif

		FScreenPassRenderTarget Output = InOutInputs.OverrideOutput;
		if (!Output.IsValid())
		{
			Output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, SceneColorInput, View.GetOverwriteLoadAction(), TEXT("FCompushadyBlitterViewExtension::PostProcessAfterMotionBlur_RenderThread"));
		}

		AddCopyTexturePass(GraphBuilder, SceneColorInput.Texture, Output.Texture, FRHICopyTextureInfo());

		if (View.bIsSceneCapture || View.bIsSceneCaptureCube || View.bIsReflectionCapture || View.bIsPlanarReflection)
		{
			return Output;
		}

		TArray<FCompushadyBlitterDrawable> CurrentDrawables;
		{
			FScopeLock DrawablesLock(&AfterMotionBlurDrawablesCriticalSection);
			CurrentDrawables = AfterMotionBlurDrawables;
		}

		if (CurrentDrawables.Num() > 0)
		{
			const FIntRect ViewRect = GetScreenSize(View, false);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("FCompushadyDrawerViewExtension::PostProcessAfterMotionBlur_RenderThread"),
				ERDGPassFlags::None,
				[this, ViewRect, Output, CurrentDrawables](FRHICommandList& RHICmdList)
				{
					FCompushadyRasterizerConfig RasterizerConfig;
					RasterizerConfig.BlendMode = ECompushadyRasterizerBlendMode::AlphaBlending;

					FTextureRHIRef RenderTarget = Output.Texture->GetRHI();
					Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyDrawerViewExtension::PostProcessAfterMotionBlur_RenderThread"),
						RHICmdList, VertexShaderRef, PixelShaderRef, &ViewRect, RenderTarget, [&]()
						{
							DrawDrawables_RenderThread(RHICmdList, ViewRect, CurrentDrawables);
						}, RasterizerConfig);
				});
		}

		return Output;
	}

	void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override
	{
		if (Pass == EPostProcessingPass::MotionBlur && bIsPassEnabled && AfterMotionBlurDrawables.Num() > 0)
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyBlitterViewExtension::PostProcessAfterMotionBlur_RenderThread));
		}
	}
#endif

	void AddDrawable(const FCompushadyBlitterDrawable& Drawable)
	{
		FScopeLock DrawablesLock(&DrawablesCriticalSection);

		Drawables.Add(Drawable);
	}

	void AddBeforePostProcessingDrawable(const FCompushadyBlitterDrawable& Drawable)
	{
		FScopeLock DrawablesLock(&BeforePostProcessingDrawablesCriticalSection);

		BeforePostProcessingDrawables.Add(Drawable);
	}

	void AddAfterMotionBlurDrawable(const FCompushadyBlitterDrawable& Drawable)
	{
		FScopeLock DrawablesLock(&AfterMotionBlurDrawablesCriticalSection);

		AfterMotionBlurDrawables.Add(Drawable);
	}

	const FMatrix& GetViewMatrix() const
	{
		return CurrentViewMatrix;
	}

	const FMatrix& GetProjectionMatrix() const
	{
		return CurrentProjectionMatrix;
	}

	void RemoveDrawable(const FGuid& Guid)
	{
		{
			FScopeLock DrawablesLock(&DrawablesCriticalSection);
			int32 FoundIndex = -1;
			for (int32 DrawableIndex = 0; DrawableIndex < Drawables.Num(); DrawableIndex++)
			{
				if (Drawables[DrawableIndex].Guid == Guid)
				{
					FoundIndex = DrawableIndex;
					break;
				}
			}

			if (FoundIndex > -1)
			{
				Drawables.RemoveAt(FoundIndex);
				return;
			}
		}

		{
			FScopeLock DrawablesLock(&BeforePostProcessingDrawablesCriticalSection);
			int32 FoundIndex = -1;
			for (int32 DrawableIndex = 0; DrawableIndex < BeforePostProcessingDrawables.Num(); DrawableIndex++)
			{
				if (BeforePostProcessingDrawables[DrawableIndex].Guid == Guid)
				{
					FoundIndex = DrawableIndex;
					break;
				}
			}

			if (FoundIndex > -1)
			{
				BeforePostProcessingDrawables.RemoveAt(FoundIndex);
				return;
			}
		}

		{
			FScopeLock DrawablesLock(&AfterMotionBlurDrawablesCriticalSection);
			int32 FoundIndex = -1;
			for (int32 DrawableIndex = 0; DrawableIndex < AfterMotionBlurDrawables.Num(); DrawableIndex++)
			{
				if (AfterMotionBlurDrawables[DrawableIndex].Guid == Guid)
				{
					FoundIndex = DrawableIndex;
					break;
				}
			}

			if (FoundIndex > -1)
			{
				AfterMotionBlurDrawables.RemoveAt(FoundIndex);
				return;
			}
		}
	}

protected:
	FCriticalSection DrawablesCriticalSection;
	TArray<FCompushadyBlitterDrawable> Drawables;

	FCriticalSection BeforePostProcessingDrawablesCriticalSection;
	TArray<FCompushadyBlitterDrawable> BeforePostProcessingDrawables;

	FCriticalSection AfterMotionBlurDrawablesCriticalSection;
	TArray<FCompushadyBlitterDrawable> AfterMotionBlurDrawables;

	FVertexShaderRHIRef VertexShaderRef;
	FCompushadyResourceBindings VSResourceBindings;

	FPixelShaderRHIRef PixelShaderRef;
	FCompushadyResourceBindings PSResourceBindings;

	FUniformBufferLayoutRHIRef UniformBufferLayoutRef;
	FUniformBufferRHIRef UniformBufferRef;
	FSamplerStateRHIRef SamplerStateRef;

	FMatrix CurrentViewMatrix;
	FMatrix CurrentProjectionMatrix;
};

bool UCompushadyBlitterSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* CurrentWorld = Cast<UWorld>(Outer);
	if (CurrentWorld && (CurrentWorld->WorldType == EWorldType::Game || CurrentWorld->WorldType == EWorldType::PIE || CurrentWorld->WorldType == EWorldType::GamePreview))
	{
		return (RHIGetInterfaceType() == ERHIInterfaceType::D3D12 || RHIGetInterfaceType() == ERHIInterfaceType::Vulkan);
	}
	return false;
}

void UCompushadyBlitterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FString ErrorMessages;

	FCompushadyResourceBindings VSResourceBindings;
	FVertexShaderRHIRef VertexShaderRef = Compushady::Utils::CreateVertexShaderFromHLSL(
		"static const float2 uvs[6] = { float2(0, 0), float2(1, 0), float2(0, 1), float2(1, 0), float2(1, 1), float2(0, 1) };"
		"struct Quad { float4 vertices[6]; };"
		"struct Output { float4 position : SV_Position; float2 uv : UV; };"
		"ConstantBuffer<Quad> quad;"
		"Output main(const uint vid : SV_VertexID) { Output o; o.position = quad.vertices[vid]; o.uv = uvs[vid]; return o; }",
		"main", VSResourceBindings, ErrorMessages);

	if (!VertexShaderRef)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unable to initialize Compushady Blitter: %s"), *ErrorMessages);
		return;
	}

	FCompushadyResourceBindings PSResourceBindings;
	FPixelShaderRHIRef PixelShaderRef = Compushady::Utils::CreatePixelShaderFromHLSL(
		"Texture2D<float4> texture0;"
		"SamplerState sampler0;"
		"struct Input { float4 position : SV_Position; float2 uv : UV; };"
		"float4 main(Input i) : SV_Target0 { return texture0.Sample(sampler0, i.uv); }",
		"main", PSResourceBindings, ErrorMessages);

	if (!PixelShaderRef)
	{
		UE_LOG(LogCompushady, Error, TEXT("Unable to initialize Compushady Blitter: %s"), *ErrorMessages);
		return;
	}

	ViewExtension = FSceneViewExtensions::NewExtension<FCompushadyBlitterViewExtension>(VertexShaderRef, VSResourceBindings, PixelShaderRef, PSResourceBindings);
}

void UCompushadyBlitterSubsystem::Deinitialize()
{

}

FGuid UCompushadyBlitterSubsystem::AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	if (!ViewExtension || !Resource || !Resource->GetTextureRHI())
	{
		return FGuid();
	}

	FCompushadyBlitterDrawable Drawable;
	Drawable.Guid = FGuid::NewGuid();
	Drawable.Texture = Resource->GetTextureRHI();
	Drawable.SRV = nullptr;
	UCompushadySRV* SRV = Cast<UCompushadySRV>(Resource);
	if (SRV)
	{
		Drawable.SRV = SRV->GetRHI();
	}
	Drawable.Quad = Quad;

	Drawable.AspectRatio = KeepAspectRatio;

	ViewExtension->AddDrawable(Drawable);

	return Drawable.Guid;
}

FGuid UCompushadyBlitterSubsystem::AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	if (!ViewExtension || !Resource || !Resource->GetTextureRHI())
	{
		return FGuid();
	}

	FCompushadyBlitterDrawable Drawable;
	Drawable.Guid = FGuid::NewGuid();
	Drawable.Texture = Resource->GetTextureRHI();
	Drawable.SRV = nullptr;
	UCompushadySRV* SRV = Cast<UCompushadySRV>(Resource);
	if (SRV)
	{
		Drawable.SRV = SRV->GetRHI();
	}
	Drawable.Quad = Quad;
	Drawable.AspectRatio = KeepAspectRatio;

	ViewExtension->AddBeforePostProcessingDrawable(Drawable);

	return Drawable.Guid;
}

FGuid UCompushadyBlitterSubsystem::AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	if (!ViewExtension || !Resource || !Resource->GetTextureRHI())
	{
		return FGuid();
	}

	FCompushadyBlitterDrawable Drawable;
	Drawable.Guid = FGuid::NewGuid();
	Drawable.Texture = Resource->GetTextureRHI();
	Drawable.SRV = nullptr;
	UCompushadySRV* SRV = Cast<UCompushadySRV>(Resource);
	if (SRV)
	{
		Drawable.SRV = SRV->GetRHI();
	}
	Drawable.Quad = Quad;
	Drawable.AspectRatio = KeepAspectRatio;

	ViewExtension->AddAfterMotionBlurDrawable(Drawable);

	return Drawable.Guid;
}

void UCompushadyBlitterSubsystem::RemoveDrawable(const FGuid& Guid)
{
	if (!ViewExtension)
	{
		return;
	}

	ViewExtension->RemoveDrawable(Guid);
}

FGuid UCompushadyBlitterSubsystem::AddViewExtension(TSharedPtr<ICompushadyTransientBlendable, ESPMode::ThreadSafe> InViewExtension, TScriptInterface<IBlendableInterface> BlendableToTrack)
{
	FGuid NewGuid = FGuid::NewGuid();
	AdditionalViewExtensions.Add(NewGuid, InViewExtension);
	TrackedBlendables.Add(NewGuid, BlendableToTrack);
	return NewGuid;
}

void UCompushadyBlitterSubsystem::RemoveViewExtension(const FGuid& Guid)
{
	if (AdditionalViewExtensions.Contains(Guid))
	{
		AdditionalViewExtensions[Guid]->Disable();
		AdditionalViewExtensions.Remove(Guid);
	}
	TrackedBlendables.Remove(Guid);
}

const FMatrix& UCompushadyBlitterSubsystem::GetViewMatrix() const
{
	if (!ViewExtension)
	{
		return FMatrix::Identity;
	}

	return ViewExtension->GetViewMatrix();
}

const FMatrix& UCompushadyBlitterSubsystem::GetProjectionMatrix() const
{
	if (!ViewExtension)
	{
		return FMatrix::Identity;
	}

	return ViewExtension->GetProjectionMatrix();
}

// let's put them here to avoid circular includes
FGuid UCompushadyResource::Draw(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddDrawable(this, Quad, KeepAspectRatio);
}

FGuid UCompushadyResource::DrawBeforePostProcessing(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddBeforePostProcessingDrawable(this, Quad, KeepAspectRatio);
}

FGuid UCompushadyResource::DrawAfterMotionBlur(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddAfterMotionBlurDrawable(this, Quad, KeepAspectRatio);
}

bool UCompushadyCBV::SetProjectionMatrixFromViewport(UObject* WorldContextObject, const int64 Offset, const bool bTranspose, const bool bInverse)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FMatrix44f Matrix = FMatrix44f(WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->GetProjectionMatrix());
		if (bInverse)
		{
			Matrix = Matrix.Inverse();
		}
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
		return true;
	}
	return false;
}

bool UCompushadyCBV::SetViewMatrixFromViewport(UObject* WorldContextObject, const int64 Offset, const bool bTranspose, const bool bInverse)
{
	if (IsValidOffset(Offset, 16 * sizeof(float)))
	{
		FMatrix44f Matrix = FMatrix44f(WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->GetViewMatrix());
		if (bInverse)
		{
			Matrix = Matrix.Inverse();
		}
		FMemory::Memcpy(BufferData.GetData() + Offset, bTranspose ? Matrix.GetTransposed().M : Matrix.M, 16 * sizeof(float));
		bBufferDataDirty = true;
		return true;
	}
	return false;
}