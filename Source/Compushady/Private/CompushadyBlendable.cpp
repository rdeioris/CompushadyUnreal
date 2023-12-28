// Copyright 2023 - Roberto De Ioris.


#include "CompushadyBlendable.h"
#include "PostProcess/DrawRectangle.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "PixelShaderUtils.h"
#include "SceneViewExtension.h"
#include "ScreenPass.h"

BEGIN_SHADER_PARAMETER_STRUCT(FCompushadyPixelShaderParameters, )
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

class FCompushadyPostProcess : public ISceneViewExtension, public TSharedFromThis<FCompushadyPostProcess, ESPMode::ThreadSafe>
{
public:
	FCompushadyPostProcess(FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray) :
		PixelShaderRef(InPixelShaderRef),
		PSResourceBindings(InPSResourceBindings),
		PSResourceArray(InPSResourceArray)
	{

	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
	{
		if (Pass == EPostProcessingPass::Tonemap && PixelShaderRef.IsValid())
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyPostProcess::PostProcessCallback_RenderThread));
		}
	}

	FScreenPassTexture PostProcessCallback_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
	{
		FScreenPassTexture SceneColor = InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor);

		FScreenPassRenderTarget Output = InOutInputs.OverrideOutput;

		if (!Output.IsValid())
		{
			Output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, SceneColor, View.GetOverwriteLoadAction(), TEXT("Compushady PostProcessing Target"));
		}

		const FIntRect OutputRect = Output.ViewRect;

		//UE_LOG(LogTemp, Error, TEXT("Before Output: %d %d"), Output.ViewRect.Width(), Output.ViewRect.Height());

		FCompushadyPixelShaderParameters* Parameters = GraphBuilder.AllocParameters<FCompushadyPixelShaderParameters>();
		Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();

		FRDGTextureRef OutputTexture = Output.Texture;

		//UE_LOG(LogTemp, Error, TEXT("After Output: %d %d"), Output.ViewRect.Width(), Output.ViewRect.Height());

		//AddClearRenderTargetPass(GraphBuilder, Output.Texture, FLinearColor::Red);

		//InOutInputs.SceneTextures.SceneTextures->GetContents()->


		GraphBuilder.AddPass(
			RDG_EVENT_NAME("CompushadyPostProcess"),
			Parameters,
			ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
			[this, &View, OutputRect, OutputTexture, InOutInputs](FRHICommandList& RHICmdList)
			{

				FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
				TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);
				FRHIBlendState* BlendState = FScreenPassPipelineState::FDefaultBlendState::GetRHI();
				FRHIDepthStencilState* DepthStencilState = FScreenPassPipelineState::FDefaultDepthStencilState::GetRHI();

				FRHIRenderPassInfo PassInfo(OutputTexture->GetRHI(), ERenderTargetActions::Load_Store);
				RHICmdList.BeginRenderPass(PassInfo, TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"));
				{
					RHICmdList.SetViewport(OutputRect.Min.X, OutputRect.Min.Y, 0.0f, OutputRect.Max.X, OutputRect.Max.Y, 1.0f);

					FGraphicsPipelineStateInitializer GraphicsPSOInit;
					RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
					GraphicsPSOInit.BlendState = BlendState;
					GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
					GraphicsPSOInit.DepthStencilState = DepthStencilState;
					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShaderRef;
					GraphicsPSOInit.PrimitiveType = PT_TriangleList;

					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

					Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, InOutInputs);

					UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, OutputRect.Min.X, OutputRect.Min.Y, OutputRect.Width(), OutputRect.Height(),
						0, 0, 1, 1,
						FIntPoint(OutputRect.Width(), OutputRect.Height()),
						FIntPoint(1, 1),
						INDEX_NONE,
						false, EDRF_UseTriangleOptimization);
				}
				RHICmdList.EndRenderPass();


			});

		return Output;
	}

	FPixelShaderRHIRef PixelShaderRef;
	FCompushadyResourceBindings PSResourceBindings;
	FCompushadyResourceArray PSResourceArray;
};

bool UCompushadyBlendable::InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromHLSL(ShaderCode, EntryPoint, PSResourceBindings, ErrorMessages);
	if (!PixelShaderRef.IsValid() || !PixelShaderRef->IsValid())
	{
		return false;
	}

	return true;
}

void UCompushadyBlendable::OverrideBlendableSettings(class FSceneView& View, float Weight) const
{
	TArray<FSceneViewExtensionRef>& ViewExtensions = ((FSceneViewFamily*)View.Family)->ViewExtensions;
	ViewExtensions.Add(MakeShared<FCompushadyPostProcess>(PixelShaderRef, PSResourceBindings, PSResourceArray));
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