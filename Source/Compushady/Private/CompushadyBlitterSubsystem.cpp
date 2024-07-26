// Copyright 2023-2024 - Roberto De Ioris.

#include "CompushadyBlitterSubsystem.h"
#include "SceneViewExtension.h"

struct FCompushadyBlitterDrawable
{
	FTextureRHIRef Texture;
	FVector4 Quad;
	ECompushadyKeepAspectRatio AspectRatio;
};

class FCompushadyBlitterViewExtension : public FSceneViewExtensionBase
{
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
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	void DrawDrawables_RenderThread(FRHICommandList& RHICmdList, FTextureRHIRef RenderTarget, const TArray<FCompushadyBlitterDrawable>& CurrentDrawables)
	{
		TStaticArray<float, 24> QuadData;

		for (const FCompushadyBlitterDrawable& Drawable : CurrentDrawables)
		{
			FVector4 Quad = Drawable.Quad;

			if (Drawable.AspectRatio != ECompushadyKeepAspectRatio::None)
			{
				const float ScreenWidth = static_cast<float>(RenderTarget->GetSizeX());
				const float ScreenHeight = static_cast<float>(RenderTarget->GetSizeY());
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
					return { nullptr, Drawable.Texture };
				},
				[](const int32 Index)
				{
					return nullptr;
				},
				[&](const int32 Index)
				{
					return SamplerStateRef;
				});

			Compushady::Utils::SetupPipelineParametersRHI(RHICmdList, PixelShaderRef, PSResourceBindings,
				[&](const int32 Index)
				{
					return UniformBufferRef;
				},
				[&](const int32 Index) -> TPair<FShaderResourceViewRHIRef, FTextureRHIRef>
				{
					RHICmdList.Transition(FRHITransitionInfo(Drawable.Texture, ERHIAccess::Unknown, ERHIAccess::SRVMask));
					return { nullptr, Drawable.Texture };
				},
				[](const int32 Index)
				{
					return nullptr;
				},
				[&](const int32 Index)
				{
					return SamplerStateRef;
				});

			RHICmdList.DrawPrimitive(0, 2, 1);
		}
	}

	void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override
	{
		TArray<FCompushadyBlitterDrawable> CurrentDrawables;
		{
			FScopeLock DrawablesLock(&DrawablesCriticalSection);
			CurrentDrawables = Drawables;
		}

		if (CurrentDrawables.Num() == 0)
		{
			return;
		}

		FTexture2DRHIRef RenderTarget = InView.Family->RenderTarget->GetRenderTargetTexture();

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FCompushadyDrawerViewExtension::PostRenderView_RenderThread"),
			ERDGPassFlags::None,
			[this, RenderTarget, CurrentDrawables](FRHICommandList& RHICmdList)
			{
				Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyDrawerViewExtension::PostRenderView_RenderThread"),
					RHICmdList, VertexShaderRef, PixelShaderRef, RenderTarget, [&]()
					{
						DrawDrawables_RenderThread(RHICmdList, RenderTarget, CurrentDrawables);
					});
			});
	}

	void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override
	{
		TArray<FCompushadyBlitterDrawable> CurrentDrawables;
		{
			FScopeLock DrawablesLock(&BeforePostProcessingDrawablesCriticalSection);
			CurrentDrawables = BeforePostProcessingDrawables;
		}

		if (CurrentDrawables.Num() == 0)
		{
			return;
		}

		TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures = *((TRDGUniformBufferRef<FSceneTextureUniformParameters>*) & Inputs);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FCompushadyDrawerViewExtension::PrePostProcessPass_RenderThread"),
			ERDGPassFlags::None,
			[this, SceneTextures, CurrentDrawables](FRHICommandList& RHICmdList)
			{
				FTexture2DRHIRef RenderTarget = SceneTextures->GetContents()->SceneColorTexture->GetRHI();
				Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyDrawerViewExtension::PrePostProcessPass_RenderThread"),
					RHICmdList, VertexShaderRef, PixelShaderRef, RenderTarget, [&]()
					{
						DrawDrawables_RenderThread(RHICmdList, RenderTarget, CurrentDrawables);
					});
			});
	}

	FScreenPassTexture PostProcessAfterMotionBlur_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
	{
		FScreenPassTexture Output = InOutInputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);

		TArray<FCompushadyBlitterDrawable> CurrentDrawables;
		{
			FScopeLock DrawablesLock(&AfterMotionBlurDrawablesCriticalSection);
			CurrentDrawables = AfterMotionBlurDrawables;
		}

		if (CurrentDrawables.Num() == 0)
		{
			return Output;
		}

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FCompushadyDrawerViewExtension::PostProcessAfterMotionBlur_RenderThread"),
			ERDGPassFlags::None,
			[this, Output, CurrentDrawables](FRHICommandList& RHICmdList)
			{
				FTexture2DRHIRef RenderTarget = Output.Texture->GetRHI();
				Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyDrawerViewExtension::PostProcessAfterMotionBlur_RenderThread"),
					RHICmdList, VertexShaderRef, PixelShaderRef, RenderTarget, [&]()
					{
						DrawDrawables_RenderThread(RHICmdList, RenderTarget, CurrentDrawables);
					});
			});

		return Output;
	}

	void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override
	{
		if (Pass == EPostProcessingPass::MotionBlur && bIsPassEnabled)
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyBlitterViewExtension::PostProcessAfterMotionBlur_RenderThread));
		}
	}

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
};

void UCompushadyBlitterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FString ErrorMessages;

	FCompushadyResourceBindings VSResourceBindings;
	FVertexShaderRHIRef VertexShaderRef = Compushady::Utils::CreateVertexShaderFromHLSL(
		"static const float2 uvs[6] = { float2(0, 0), float2(1, 0), float2(0, 1), float2(1, 0), float2(1, 1), float2(0, 1) };"
		"struct Quad { float4 vertices[6]; };"
		"struct Output { float4 position : SV_Position; float2 uv : UV; };"
		"ConstantBuffer<Quad> quad;"
		"Output main(const uint vid : SV_VertexID) { Output o; o.position = quad.vertices[vid], 0, 1; o.uv = uvs[vid]; return o; }",
		"main", VSResourceBindings, ErrorMessages);

	if (!VertexShaderRef)
	{
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
		return;
	}

	ViewExtension = FSceneViewExtensions::NewExtension<FCompushadyBlitterViewExtension>(VertexShaderRef, VSResourceBindings, PixelShaderRef, PSResourceBindings);
}

void UCompushadyBlitterSubsystem::Deinitialize()
{

}

void UCompushadyBlitterSubsystem::AddDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	if (!ViewExtension || !Resource || !Resource->GetTextureRHI())
	{
		return;
	}

	FCompushadyBlitterDrawable Drawable;
	Drawable.Texture = Resource->GetTextureRHI();
	Drawable.Quad = Quad;
	Drawable.AspectRatio = KeepAspectRatio;

	ViewExtension->AddDrawable(Drawable);
}

void UCompushadyBlitterSubsystem::AddBeforePostProcessingDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	if (!ViewExtension || !Resource || !Resource->GetTextureRHI())
	{
		return;
	}

	FCompushadyBlitterDrawable Drawable;
	Drawable.Texture = Resource->GetTextureRHI();
	Drawable.Quad = Quad;
	Drawable.AspectRatio = KeepAspectRatio;

	ViewExtension->AddBeforePostProcessingDrawable(Drawable);
}

void UCompushadyBlitterSubsystem::AddAfterMotionBlurDrawable(UCompushadyResource* Resource, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	if (!ViewExtension || !Resource || !Resource->GetTextureRHI())
	{
		return;
	}

	FCompushadyBlitterDrawable Drawable;
	Drawable.Texture = Resource->GetTextureRHI();
	Drawable.Quad = Quad;
	Drawable.AspectRatio = KeepAspectRatio;

	ViewExtension->AddAfterMotionBlurDrawable(Drawable);
}

// let's put them here to avoid circular includes
void UCompushadyResource::Draw(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddDrawable(this, Quad, KeepAspectRatio);
}

void UCompushadyResource::DrawBeforePostProcessing(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddBeforePostProcessingDrawable(this, Quad, KeepAspectRatio);
}

void UCompushadyResource::DrawAfterMotionBlur(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddAfterMotionBlurDrawable(this, Quad, KeepAspectRatio);
}