// Copyright 2023 - Roberto De Ioris.


#include "CompushadyBlendable.h"

#if COMPUSHADY_UE_VERSION >= 53
#include "PostProcess/PostProcessMaterialInputs.h"
#include "PostProcess/DrawRectangle.h"
#include "SceneViewExtension.h"
#include "SystemTextures.h"

#include "CompushadyBlitterSubsystem.h"

class ICompushadyViewExtension
{
public:
	void SetPriority(const int32 NewPriority)
	{
		CompushadyPriority = NewPriority;
	}

protected:

	ICompushadyViewExtension(FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray) :
		PixelShaderRef(InPixelShaderRef),
		PSResourceBindings(InPSResourceBindings),
		PSResourceArray(InPSResourceArray)
	{
	}

	void FillSceneTextures(FCompushadySceneTextures& SceneTextures, FRHICommandList& RHICmdList, FTextureRHIRef SceneColorInput, const FSceneTextureUniformParameters* Contents)
	{
		SceneTextures.SetTexture(ECompushadySceneTexture::SceneColorInput, SceneColorInput);

		SceneTextures.SetTexture(ECompushadySceneTexture::SceneColor, Contents->SceneColorTexture->GetRHI());

		FRHIViewDesc::FTextureSRV::FInitializer SRVViewDesc = FRHIViewDesc::CreateTextureSRV();
		SRVViewDesc.SetDimensionFromTexture(Contents->SceneDepthTexture->GetRHI());

		SRVViewDesc.SetPlane(ERHITexturePlane::Depth);
		FShaderResourceViewRHIRef DepthSRV = COMPUSHADY_CREATE_SRV(Contents->SceneDepthTexture->GetRHI(), SRVViewDesc);
		SceneTextures.SetSRV(ECompushadySceneTexture::Depth, DepthSRV);

		SRVViewDesc.SetPlane(ERHITexturePlane::Stencil);
		FShaderResourceViewRHIRef StencilSRV = COMPUSHADY_CREATE_SRV(Contents->SceneDepthTexture->GetRHI(), SRVViewDesc);
		SceneTextures.SetSRV(ECompushadySceneTexture::Stencil, StencilSRV);

		SceneTextures.SetTexture(ECompushadySceneTexture::CustomDepth, Contents->CustomDepthTexture->GetRHI());
		SceneTextures.SetSRV(ECompushadySceneTexture::CustomStencil, Contents->CustomStencilTexture->GetRHI());
		SceneTextures.SetTexture(ECompushadySceneTexture::GBufferA, Contents->GBufferATexture->GetRHI());
		SceneTextures.SetTexture(ECompushadySceneTexture::GBufferB, Contents->GBufferBTexture->GetRHI());
		SceneTextures.SetTexture(ECompushadySceneTexture::GBufferC, Contents->GBufferCTexture->GetRHI());
		SceneTextures.SetTexture(ECompushadySceneTexture::GBufferD, Contents->GBufferDTexture->GetRHI());
		SceneTextures.SetTexture(ECompushadySceneTexture::GBufferE, Contents->GBufferETexture->GetRHI());
		SceneTextures.SetTexture(ECompushadySceneTexture::GBufferF, Contents->GBufferFTexture->GetRHI());
		SceneTextures.SetTexture(ECompushadySceneTexture::Velocity, Contents->GBufferVelocityTexture->GetRHI());
	}

	FScreenPassTexture PostProcessCallback_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
	{
#if COMPUSHADY_UE_VERSION >= 54
		const FScreenPassTexture& SceneColorInput = FScreenPassTexture::CopyFromSlice(GraphBuilder, InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor));
#else
		FScreenPassTexture SceneColorInput = InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor);
#endif

		FScreenPassRenderTarget Output = InOutInputs.OverrideOutput;
		if (!Output.IsValid())
		{
			Output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, SceneColorInput, View.GetOverwriteLoadAction(), TEXT("ICompushadyViewExtension::PostProcessCallback_RenderThread"));
		}

		GraphBuilder.ConvertToExternalTexture(Output.Texture);

		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
		TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
			ERDGPassFlags::None,
			[this, VertexShader, Output, SceneColorInput, InOutInputs](FRHICommandList& RHICmdList)
			{
				FTextureRHIRef RenderTarget = Output.Texture->GetRHI();

				FCompushadySceneTextures SceneTextures = {};
				FillSceneTextures(SceneTextures, RHICmdList, SceneColorInput.Texture->GetRHI(), InOutInputs.SceneTextures.SceneTextures->GetContents());

				Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
					RHICmdList, VertexShader.GetVertexShader(), PixelShaderRef, RenderTarget, [&]()
					{
						Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures);
						UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, 0, 0, RenderTarget->GetSizeX(), RenderTarget->GetSizeY(),
							0, 0, 1, 1,
							RenderTarget->GetSizeXY(),
							FIntPoint(1, 1),
							INDEX_NONE,
							false, EDRF_UseTriangleOptimization);
					});
			});

		return Output;
	}

	FPixelShaderRHIRef PixelShaderRef;
	FCompushadyResourceBindings PSResourceBindings;
	FCompushadyResourceArray PSResourceArray;
	int32 CompushadyPriority = 0;
};

class FCompushadyViewExtension : public ISceneViewExtension, public TSharedFromThis<FCompushadyViewExtension, ESPMode::ThreadSafe>, public ICompushadyViewExtension
{
public:
	FCompushadyViewExtension(FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray, const ECompushadyPostProcessLocation PostProcessLocation) :
		ICompushadyViewExtension(InPixelShaderRef, InPSResourceBindings, InPSResourceArray)
	{
		switch (PostProcessLocation)
		{
		case ECompushadyPostProcessLocation::PrePostProcess:
			bPrePostProcess = true;
			break;
		case ECompushadyPostProcessLocation::AfterMotionBlur:
			RequiredPass = EPostProcessingPass::MotionBlur;
			break;
		case ECompushadyPostProcessLocation::AfterTonemapping:
		default:
			RequiredPass = EPostProcessingPass::Tonemap;
			break;
		}
	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
	{
		if (!bPrePostProcess && Pass == RequiredPass && bIsPassEnabled)
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyViewExtension::PostProcessCallback_RenderThread));
		}
	}

	void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override
	{
		if (!bPrePostProcess)
		{
			return;
		}

		TRDGUniformBufferRef<FSceneTextureUniformParameters> InputSceneTextures = *((TRDGUniformBufferRef<FSceneTextureUniformParameters>*) & Inputs);

		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
		TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
			ERDGPassFlags::None,
			[this, VertexShader, InputSceneTextures](FRHICommandList& RHICmdList)
			{
				FTextureRHIRef RenderTarget = InputSceneTextures->GetContents()->SceneColorTexture->GetRHI();

				FCompushadySceneTextures SceneTextures = {};
				FillSceneTextures(SceneTextures, RHICmdList, RenderTarget, InputSceneTextures->GetContents());

				Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
					RHICmdList, VertexShader.GetVertexShader(), PixelShaderRef, RenderTarget, [&]()
					{
						Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures);
						UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, 0, 0, RenderTarget->GetSizeX(), RenderTarget->GetSizeY(),
							0, 0, 1, 1,
							RenderTarget->GetSizeXY(),
							FIntPoint(1, 1),
							INDEX_NONE,
							false, EDRF_UseTriangleOptimization);
					});
			});
	}

	virtual int32 GetPriority() const override { return CompushadyPriority; }

protected:
	EPostProcessingPass RequiredPass = EPostProcessingPass::Tonemap;
	bool bPrePostProcess = false;
};

class FCompushadyPostProcess : public FSceneViewExtensionBase, public ICompushadyViewExtension
{
public:
	FCompushadyPostProcess(const FAutoRegister& AutoRegister, FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray, const ECompushadyPostProcessLocation PostProcessLocation) :
		FSceneViewExtensionBase(AutoRegister),
		ICompushadyViewExtension(InPixelShaderRef, InPSResourceBindings, InPSResourceArray)
	{
		switch (PostProcessLocation)
		{
		case ECompushadyPostProcessLocation::PrePostProcess:
			bPrePostProcess = true;
			break;
		case ECompushadyPostProcessLocation::AfterMotionBlur:
			RequiredPass = EPostProcessingPass::MotionBlur;
			break;
		case ECompushadyPostProcessLocation::AfterTonemapping:
		default:
			RequiredPass = EPostProcessingPass::Tonemap;
			break;
		}
	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
	{
		if (!bPrePostProcess && Pass == RequiredPass && bIsPassEnabled)
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyPostProcess::PostProcessCallback_RenderThread));
		}
	}

	void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override
	{
		if (!bPrePostProcess)
		{
			return;
		}

		TRDGUniformBufferRef<FSceneTextureUniformParameters> InputSceneTextures = *((TRDGUniformBufferRef<FSceneTextureUniformParameters>*) & Inputs);

		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
		TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
			ERDGPassFlags::None,
			[this, VertexShader, InputSceneTextures](FRHICommandList& RHICmdList)
			{
				FTexture2DRHIRef RenderTarget = InputSceneTextures->GetContents()->SceneColorTexture->GetRHI();

				FCompushadySceneTextures SceneTextures = {};
				FillSceneTextures(SceneTextures, RHICmdList, InputSceneTextures->GetContents()->SceneColorTexture->GetRHI(), InputSceneTextures->GetContents());

				Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
					RHICmdList, VertexShader.GetVertexShader(), PixelShaderRef, RenderTarget, [&]()
					{
						Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures);
						UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, 0, 0, RenderTarget->GetSizeX(), RenderTarget->GetSizeY(),
							0, 0, 1, 1,
							RenderTarget->GetSizeXY(),
							FIntPoint(1, 1),
							INDEX_NONE,
							false, EDRF_UseTriangleOptimization);
					});
			});
	}

	virtual int32 GetPriority() const override { return CompushadyPriority; }

protected:
	EPostProcessingPass RequiredPass = EPostProcessingPass::Tonemap;
	bool bPrePostProcess = false;
};
#endif

bool UCompushadyBlendable::InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages)
{
	PostProcessLocation = InPostProcessLocation;
	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromHLSL(ShaderCode, EntryPoint, PSResourceBindings, ErrorMessages);
	return PixelShaderRef.IsValid();
}

bool UCompushadyBlendable::InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages)
{
	PostProcessLocation = InPostProcessLocation;
	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromGLSL(ShaderCode, EntryPoint, PSResourceBindings, ErrorMessages);
	return PixelShaderRef.IsValid();
}

void UCompushadyBlendable::OverrideBlendableSettings(class FSceneView& View, float Weight) const
{
#if COMPUSHADY_UE_VERSION >= 53
	TArray<FSceneViewExtensionRef>& ViewExtensions = ((FSceneViewFamily*)View.Family)->ViewExtensions;
	ViewExtensions.Add(MakeShared<FCompushadyViewExtension>(PixelShaderRef, PSResourceBindings, PSResourceArray, PostProcessLocation));
#endif
}

bool UCompushadyBlendable::UpdateResources(const FCompushadyResourceArray& InPSResourceArray, FString& ErrorMessages)
{
	if (!Compushady::Utils::ValidateResourceBindings(InPSResourceArray, PSResourceBindings, ErrorMessages))
	{
		return false;
	}

	UntrackResources();
	PSResourceArray = InPSResourceArray;
	TrackResources(PSResourceArray);

	return true;
}

bool UCompushadyBlendable::UpdateResourcesByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages)
{
	FCompushadyResourceArray InPSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(PSResourceMap, PSResourceBindings, InPSResourceArray, ErrorMessages))
	{
		return false;
	}

	return UpdateResources(InPSResourceArray, ErrorMessages);
}

FPixelShaderRHIRef UCompushadyBlendable::GetPixelShader() const
{
	return PixelShaderRef;
}

FGuid UCompushadyBlendable::AddToBlitter(UObject* WorldContextObject, const int32 Priority)
{
#if COMPUSHADY_UE_VERSION >= 53
	TSharedPtr<FCompushadyPostProcess, ESPMode::ThreadSafe> NewViewExtension = FSceneViewExtensions::NewExtension<FCompushadyPostProcess>(PixelShaderRef, PSResourceBindings, PSResourceArray, PostProcessLocation);
	FGuid Guid = WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddViewExtension(NewViewExtension);
	NewViewExtension->SetPriority(Priority);
	return Guid;
#else
	return FGuid::NewGuid();
#endif
}