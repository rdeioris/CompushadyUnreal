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
				FTexture2DRHIRef RenderTarget = Output.Texture->GetRHI();

				FCompushadySceneTextures SceneTextures = {};
				SceneTextures.SetTexture(ECompushadySceneTexture::SceneColorInput, SceneColorInput.Texture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::SceneColor, InOutInputs.SceneTextures.SceneTextures->GetContents()->SceneColorTexture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::Depth, InOutInputs.SceneTextures.SceneTextures->GetContents()->SceneDepthTexture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::CustomDepth, InOutInputs.SceneTextures.SceneTextures->GetContents()->CustomDepthTexture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::GBufferA, InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::GBufferB, InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferBTexture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::GBufferC, InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferCTexture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::GBufferD, InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferDTexture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::GBufferE, InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferETexture->GetRHI());
				SceneTextures.SetTexture(ECompushadySceneTexture::GBufferF, InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferFTexture->GetRHI());

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
	FCompushadyViewExtension(FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray) :
		ICompushadyViewExtension(InPixelShaderRef, InPSResourceBindings, InPSResourceArray)
	{
	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
	{
		if (Pass == EPostProcessingPass::Tonemap && bIsPassEnabled)
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyViewExtension::PostProcessCallback_RenderThread));
		}
	}

	virtual int32 GetPriority() const override { return CompushadyPriority; }
};

class FCompushadyPostProcess : public FSceneViewExtensionBase, public ICompushadyViewExtension
{
public:
	FCompushadyPostProcess(const FAutoRegister& AutoRegister, FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray) :
		FSceneViewExtensionBase(AutoRegister),
		ICompushadyViewExtension(InPixelShaderRef, InPSResourceBindings, InPSResourceArray)
	{
	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
	{
		if (Pass == EPostProcessingPass::Tonemap && bIsPassEnabled)
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyPostProcess::PostProcessCallback_RenderThread));
		}
	}

	virtual int32 GetPriority() const override { return CompushadyPriority; }
};
#endif

bool UCompushadyBlendable::InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromHLSL(ShaderCode, EntryPoint, PSResourceBindings, ErrorMessages);
	return PixelShaderRef.IsValid();
}

bool UCompushadyBlendable::InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromGLSL(ShaderCode, EntryPoint, PSResourceBindings, ErrorMessages);
	return PixelShaderRef.IsValid();
}

void UCompushadyBlendable::OverrideBlendableSettings(class FSceneView& View, float Weight) const
{
#if COMPUSHADY_UE_VERSION >= 53
	TArray<FSceneViewExtensionRef>& ViewExtensions = ((FSceneViewFamily*)View.Family)->ViewExtensions;
	ViewExtensions.Add(MakeShared<FCompushadyViewExtension>(PixelShaderRef, PSResourceBindings, PSResourceArray));
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

FPixelShaderRHIRef UCompushadyBlendable::GetPixelShader() const
{
	return PixelShaderRef;
}

FGuid UCompushadyBlendable::AddToBlitter(UObject* WorldContextObject, const int32 Priority)
{
	TSharedPtr<FCompushadyPostProcess, ESPMode::ThreadSafe> NewViewExtension = FSceneViewExtensions::NewExtension<FCompushadyPostProcess>(PixelShaderRef, PSResourceBindings, PSResourceArray);
	FGuid Guid = WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddViewExtension(NewViewExtension);
	NewViewExtension->SetPriority(Priority);
	return Guid;
}