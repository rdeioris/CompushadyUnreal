// Copyright 2023 - Roberto De Ioris.


#include "CompushadyBlendable.h"

#if COMPUSHADY_UE_VERSION >= 53
#include "PostProcess/DrawRectangle.h"
#include "PixelShaderUtils.h"
#include "SceneViewExtension.h"
#include "SceneRenderTargetParameters.h"
#include "SystemTextures.h"
#include "ScreenPass.h"

BEGIN_SHADER_PARAMETER_STRUCT(FCompushadyPixelShaderParameters, )
	SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
	//SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
	//SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, GBufferATexture)
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
		if (Pass == EPostProcessingPass::MotionBlur && PixelShaderRef.IsValid())
		{
			//InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyPostProcess::PostProcessCallback_RenderThread));
		}
	}

	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override
	{
		TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures = *((TRDGUniformBufferRef<FSceneTextureUniformParameters>*) & Inputs);
		UE_LOG(LogTemp, Error, TEXT("PrePostProcessPass_RenderThread() SceneColorTexture %s"), *(SceneTextures->GetContents()->SceneColorTexture->Desc.GetSize().ToString()));
		UE_LOG(LogTemp, Error, TEXT("PrePostProcessPass_RenderThread() GBufferATexture %s"), *(SceneTextures->GetContents()->GBufferATexture->Desc.GetSize().ToString()));
		UE_LOG(LogTemp, Error, TEXT("PrePostProcessPass_RenderThread() GBufferBTexture %s"), *(SceneTextures->GetContents()->GBufferBTexture->Desc.GetSize().ToString()));

		FCompushadyPixelShaderParameters* Parameters = GraphBuilder.AllocParameters<FCompushadyPixelShaderParameters>();
		//const FRDGSystemTextures& SystemTextures = FRDGSystemTextures::Get(GraphBuilder);
		//CreateSceneTextureUniformBuffer();
		//Parameters->GBufferATexture = SystemTextures.//InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture;
		//Parameters->SceneTextures.SceneTextures = CreateSceneTextureUniformBuffer(GraphBuilder, View);// InOutInputs.SceneTextures;
		Parameters->SceneTextures = SceneTextures;
		//Parameters->GBufferATexture = NewSceneTextures->GetContents()->GBufferATexture;
		//Parameters->View = View.ViewUniformBuffer;
		FIntVector TargetSize = SceneTextures->GetContents()->SceneColorTexture->Desc.GetSize();
		Parameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures->GetContents()->SceneColorTexture, ERenderTargetLoadAction::ELoad);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("CompushadyPostProcess"),
			Parameters,
			ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
			[this, &View, SceneTextures, TargetSize](FRHICommandList& RHICmdList)
			{

				FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
				TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);
				FRHIBlendState* BlendState = FScreenPassPipelineState::FDefaultBlendState::GetRHI();
				FRHIDepthStencilState* DepthStencilState = FScreenPassPipelineState::FDefaultDepthStencilState::GetRHI();

				FRHIRenderPassInfo PassInfo(SceneTextures->GetContents()->SceneColorTexture->GetRHI(), ERenderTargetActions::Load_Store);
				RHICmdList.BeginRenderPass(PassInfo, TEXT("FCompushadyPostProcess::PrePostProcessPass_RenderThread"));
				{
					RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);

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

					//RHICmdList.Transition(FRHITransitionInfo(InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture->GetRHI(), ERHIAccess::RTV, ERHIAccess::SRVMask));

					FPostProcessMaterialInputs PPInputs; //= InOutInputs;
					PPInputs.SceneTextures.SceneTextures = SceneTextures;// ->GetContents())->GBufferATexture = NewSceneTextures->GetContents()->GBufferATexture;

					Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, PPInputs);

					//UE_LOG(LogTemp, Error, TEXT("%f %f %f %f"), static_cast<float>(OutputRect.Min.X), static_cast<float>(OutputRect.Min.Y), static_cast<float>(OutputRect.Width()), static_cast<float>(OutputRect.Height()));

					UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, 0, 0, TargetSize.X, TargetSize.Y,
						0, 0, 1, 1,
						FIntPoint(TargetSize.X, TargetSize.Y),
						FIntPoint(1, 1),
						INDEX_NONE,
						false, EDRF_UseTriangleOptimization);

					//RHICmdList.Transition(FRHITransitionInfo(InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture->GetRHI(), ERHIAccess::SRVMask, ERHIAccess::RTV));
				}
				RHICmdList.EndRenderPass();
			});
	}

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override
	{
		return;
#if 0
		NewSceneTextures = CreateSceneTextureUniformBuffer(GraphBuilder, InView);

		FCompushadyPixelShaderParameters* Parameters = GraphBuilder.AllocParameters<FCompushadyPixelShaderParameters>();
		Parameters->SceneTextures = NewSceneTextures;
		Parameters->RenderTargets = RenderTargets;

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("CompushadyPostProcess"),
			Parameters,
			ERDGPassFlags::Raster,
			[this, &InView, SceneTextures, RenderTargets](FRHICommandList& RHICmdList)
			{

				FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(InView.GetFeatureLevel());
				TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);
				FRHIBlendState* BlendState = TStaticBlendState<>::GetRHI();// FScreenPassPipelineState::FDefaultBlendState::GetRHI();
				FRHIDepthStencilState* DepthStencilState = FScreenPassPipelineState::FDefaultDepthStencilState::GetRHI();

				//FRHIRenderPassInfo PassInfo(RenderTargets.Output[0].GetTexture()->GetRHI(), ERenderTargetActions::Load_Store);
				//RHICmdList.BeginRenderPass(PassInfo, TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"));
				//{
				//	RHICmdList.SetViewport(0, 0, 0.0f, 1024, 1024, 1.0f);

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

				//RHICmdList.Transition(FRHITransitionInfo(InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture->GetRHI(), ERHIAccess::RTV, ERHIAccess::SRVMask));

				FPostProcessMaterialInputs PPInputs;// = InOutInputs;
				PPInputs.SceneTextures.SceneTextures = NewSceneTextures;// ->GetContents())->GBufferATexture = NewSceneTextures->GetContents()->GBufferATexture;

				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, PPInputs);

				//UE_LOG(LogTemp, Error, TEXT("%f %f %f %f"), static_cast<float>(OutputRect.Min.X), static_cast<float>(OutputRect.Min.Y), static_cast<float>(OutputRect.Width()), static_cast<float>(OutputRect.Height()));

				UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, 0, 0, 1024, 1024,
					0, 0, 1, 1,
					FIntPoint(1024, 1024),
					FIntPoint(1, 1),
					INDEX_NONE,
					false, EDRF_UseTriangleOptimization);

				//RHICmdList.Transition(FRHITransitionInfo(InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture->GetRHI(), ERHIAccess::SRVMask, ERHIAccess::RTV));
			//}
			//RHICmdList.EndRenderPass();
			});
		//GBufferA = NewSceneTextures->GetContents()->GBufferATexture;
#endif
	}

	FScreenPassTexture PostProcessCallback_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
	{
#if COMPUSHADY_UE_VERSION >= 54
		const FScreenPassTexture& SceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor));
#else
		FScreenPassTexture SceneColor = InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor);
#endif

		FScreenPassRenderTarget Output = InOutInputs.OverrideOutput;

		if (!Output.IsValid())
		{
			Output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, SceneColor, View.GetOverwriteLoadAction(), TEXT("Compushady PostProcessing Target"));
		}

		const FIntRect OutputRect = Output.ViewRect;

		//UE_LOG(LogTemp, Error, TEXT("Before Output: %d %d"), Output.ViewRect.Width(), Output.ViewRect.Height());
		//CreateSceneTextureUniformBuffer
		FCompushadyPixelShaderParameters* Parameters = GraphBuilder.AllocParameters<FCompushadyPixelShaderParameters>();
		//const FRDGSystemTextures& SystemTextures = FRDGSystemTextures::Get(GraphBuilder);
		//CreateSceneTextureUniformBuffer();
		//Parameters->GBufferATexture = SystemTextures.//InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture;
		//Parameters->SceneTextures.SceneTextures = CreateSceneTextureUniformBuffer(GraphBuilder, View);// InOutInputs.SceneTextures;
		//Parameters->SceneTextures = InOutInputs.SceneTextures;
		//Parameters->GBufferATexture = NewSceneTextures->GetContents()->GBufferATexture;
		//Parameters->View = View.ViewUniformBuffer;
		Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();

		FRDGTextureRef OutputTexture = Output.Texture;

		//UE_LOG(LogTemp, Error, TEXT("After Output: %d %d"), Output.ViewRect.Width(), Output.ViewRect.Height());

		//AddClearRenderTargetPass(GraphBuilder, Output.Texture, FLinearColor::Red);

		//InOutInputs.SceneTextures.SceneTextures->GetContents()->

		//FPostProcessMaterialInputs PPInputs;
		//PPInputs.SceneTextures.SceneTextures = Parameters->SceneTextures.SceneTextures;

		//FViewUniformShaderParameters* ViewUniformShaderParameters = (*View.ViewUniformBuffer.GetReference())->;

		//UE_LOG(LogTemp, Error, TEXT("%s %s -> %s"), *(OutputTexture->Desc.GetSize().ToString()), *(OutputRect.ToString()), *(ViewUniformShaderParameters->BufferSizeAndInvSize.ToString()));



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

					//RHICmdList.Transition(FRHITransitionInfo(InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture->GetRHI(), ERHIAccess::RTV, ERHIAccess::SRVMask));

					FPostProcessMaterialInputs PPInputs = InOutInputs;
					PPInputs.SceneTextures.SceneTextures = NewSceneTextures;// ->GetContents())->GBufferATexture = NewSceneTextures->GetContents()->GBufferATexture;

					Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, PPInputs);

					//UE_LOG(LogTemp, Error, TEXT("%f %f %f %f"), static_cast<float>(OutputRect.Min.X), static_cast<float>(OutputRect.Min.Y), static_cast<float>(OutputRect.Width()), static_cast<float>(OutputRect.Height()));

					UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, OutputRect.Min.X, OutputRect.Min.Y, OutputRect.Width(), OutputRect.Height(),
						0, 0, 1, 1,
						FIntPoint(OutputRect.Width(), OutputRect.Height()),
						FIntPoint(1, 1),
						INDEX_NONE,
						false, EDRF_UseTriangleOptimization);

					//RHICmdList.Transition(FRHITransitionInfo(InOutInputs.SceneTextures.SceneTextures->GetContents()->GBufferATexture->GetRHI(), ERHIAccess::SRVMask, ERHIAccess::RTV));
				}
				RHICmdList.EndRenderPass();
			});

		return Output;
	}

	FPixelShaderRHIRef PixelShaderRef;
	FCompushadyResourceBindings PSResourceBindings;
	FCompushadyResourceArray PSResourceArray;
	TRDGUniformBufferRef<FSceneTextureUniformParameters> NewSceneTextures;
};
#endif

bool UCompushadyBlendable::InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromHLSL(ShaderCode, EntryPoint, PSResourceBindings, ErrorMessages);
	if (!PixelShaderRef)
	{
		return false;
	}

	return true;
}

bool UCompushadyBlendable::InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromGLSL(ShaderCode, EntryPoint, PSResourceBindings, ErrorMessages);
	if (!PixelShaderRef)
	{
		return false;
	}

	return true;
}

void UCompushadyBlendable::OverrideBlendableSettings(class FSceneView& View, float Weight) const
{
#if COMPUSHADY_UE_VERSION >= 53
	TArray<FSceneViewExtensionRef>& ViewExtensions = ((FSceneViewFamily*)View.Family)->ViewExtensions;
	ViewExtensions.Add(MakeShared<FCompushadyPostProcess>(PixelShaderRef, PSResourceBindings, PSResourceArray));
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