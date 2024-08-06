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

	ICompushadyViewExtension(FVertexShaderRHIRef InVertexShaderRef, const FCompushadyResourceBindings& InVSResourceBindings, const FCompushadyResourceArray& InVSResourceArray, FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray, const int32 InNumVertices, const int32 InNumInstances, const FCompushadyBlendableRasterizerConfig& InRasterizerConfig) :
		PixelShaderRef(InPixelShaderRef),
		PSResourceBindings(InPSResourceBindings),
		PSResourceArray(InPSResourceArray),
		VertexShaderRef(InVertexShaderRef),
		VSResourceBindings(InVSResourceBindings),
		VSResourceArray(InVSResourceArray),
		NumVertices(InNumVertices),
		NumInstances(InNumInstances),
		RasterizerConfig(InRasterizerConfig)
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

	void FillRasterizerConfig(const FViewMatrices& ViewMatrices, const FVector2D ScreenSize)
	{
		if (VSResourceArray.CBVs.Num() == 0)
		{
			return;
		}

		auto SetMatrixByOffset = [&](const int32 Offset, const FMatrix& Matrix)
			{
				if (Offset >= 0 && static_cast<int64>(Offset + (sizeof(float) * 16)) <= VSResourceArray.CBVs[0]->GetBufferSize())
				{
					VSResourceArray.CBVs[0]->SetMatrixFloat(Offset, Matrix, false, false);
				}
			};

		auto SetVector2ByOffset = [&](const int32 Offset, const FVector2D Vector)
			{
				if (Offset >= 0 && static_cast<int64>(Offset + (sizeof(float) * 2)) <= VSResourceArray.CBVs[0]->GetBufferSize())
				{
					VSResourceArray.CBVs[0]->SetFloat(Offset, Vector.X);
					VSResourceArray.CBVs[0]->SetFloat(Offset + sizeof(float), Vector.Y);
				}
			};

		auto SetVector4ByOffset = [&](const int32 Offset, const FVector4 Vector)
			{
				if (Offset >= 0 && static_cast<int64>(Offset + (sizeof(float) * 4)) <= VSResourceArray.CBVs[0]->GetBufferSize())
				{
					VSResourceArray.CBVs[0]->SetFloat(Offset, Vector.X);
					VSResourceArray.CBVs[0]->SetFloat(Offset + sizeof(float) * 1, Vector.Y);
					VSResourceArray.CBVs[0]->SetFloat(Offset + sizeof(float) * 2, Vector.Z);
					VSResourceArray.CBVs[0]->SetFloat(Offset + sizeof(float) * 3, Vector.W);
				}
			};

		SetMatrixByOffset(RasterizerConfig.ViewMatrixOffset, ViewMatrices.GetViewMatrix());
		SetMatrixByOffset(RasterizerConfig.ProjectionMatrixOffset, ViewMatrices.GetProjectionMatrix());
		SetMatrixByOffset(RasterizerConfig.InverseViewMatrixOffset, ViewMatrices.GetInvViewMatrix());
		SetMatrixByOffset(RasterizerConfig.InverseProjectionMatrixOffset, ViewMatrices.GetInvProjectionMatrix());
		SetVector2ByOffset(RasterizerConfig.ScreenSizeFloat2Offset, ScreenSize);

		SetMatrixByOffset(RasterizerConfig.ViewProjectionMatrixOffset, ViewMatrices.GetViewProjectionMatrix());
		SetMatrixByOffset(RasterizerConfig.InverseViewProjectionMatrixOffset, ViewMatrices.GetInvViewProjectionMatrix());

		SetVector4ByOffset(RasterizerConfig.ViewOriginFloat4Offset, ViewMatrices.GetViewOrigin());

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

		if (!VertexShaderRef)
		{
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
		}
		else
		{
			const FSceneTextureUniformParameters* SceneTextureUniform = InOutInputs.SceneTextures.SceneTextures->GetContents();

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
				ERDGPassFlags::None,
				[this, SceneColorInput, Output, &View, SceneTextureUniform](FRHICommandList& RHICmdList)
				{
					FCompushadySceneTextures SceneTextures = {};
					FillSceneTextures(SceneTextures, RHICmdList, SceneColorInput.Texture->GetRHI(), SceneTextureUniform);

					FTextureRHIRef RenderTarget = Output.Texture->GetRHI();

					FTextureRHIRef DepthStencil = nullptr;

					if (RasterizerConfig.bCheckDepth)
					{
						DepthStencil = SceneTextureUniform->SceneDepthTexture->GetRHI();
					}

					const FVector2D ScreenSize = FVector2D(RenderTarget->GetDesc().GetSize().X, RenderTarget->GetDesc().GetSize().Y);

					Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
						RHICmdList, VertexShaderRef, PixelShaderRef, RenderTarget, DepthStencil, [&]()
						{
							FillRasterizerConfig(View.ViewMatrices, ScreenSize);
							Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings);
							Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures);
							RHICmdList.DrawPrimitive(0, NumVertices / 3, NumInstances);
						});
				});
		}

		return Output;
	}

	FPixelShaderRHIRef PixelShaderRef;
	FCompushadyResourceBindings PSResourceBindings;
	FCompushadyResourceArray PSResourceArray;

	FVertexShaderRHIRef VertexShaderRef = nullptr;
	FCompushadyResourceBindings VSResourceBindings;
	FCompushadyResourceArray VSResourceArray;

	int32 NumVertices = 0;
	int32 NumInstances = 0;

	int32 CompushadyPriority = 0;

	FCompushadyBlendableRasterizerConfig RasterizerConfig;
};

class FCompushadyViewExtension : public ISceneViewExtension, public TSharedFromThis<FCompushadyViewExtension, ESPMode::ThreadSafe>, public ICompushadyViewExtension
{
public:
	FCompushadyViewExtension(FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray, const ECompushadyPostProcessLocation PostProcessLocation) :
		ICompushadyViewExtension(nullptr, {}, {}, InPixelShaderRef, InPSResourceBindings, InPSResourceArray, 0, 0, {})
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
	FCompushadyPostProcess(const FAutoRegister& AutoRegister, FVertexShaderRHIRef InVertexShaderRef, const FCompushadyResourceBindings& InVSResourceBindings, const FCompushadyResourceArray& InVSResourceArray, FPixelShaderRHIRef InPixelShaderRef, const FCompushadyResourceBindings& InPSResourceBindings, const FCompushadyResourceArray& InPSResourceArray, const ECompushadyPostProcessLocation PostProcessLocation, const int32 InNumVertices, const int32 InNumInstances, const FCompushadyBlendableRasterizerConfig& InRasterizerConfig) :
		FSceneViewExtensionBase(AutoRegister),
		ICompushadyViewExtension(InVertexShaderRef, InVSResourceBindings, InVSResourceArray, InPixelShaderRef, InPSResourceBindings, InPSResourceArray, InNumVertices, InNumInstances, InRasterizerConfig)
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

		if (!VertexShaderRef)
		{
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
		else
		{
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
				ERDGPassFlags::None,
				[this, InputSceneTextures, &View](FRHICommandList& RHICmdList)
				{
					FTextureRHIRef RenderTarget = InputSceneTextures->GetContents()->SceneColorTexture->GetRHI();
					FTextureRHIRef DepthStencil = nullptr;

					if (RasterizerConfig.bCheckDepth)
					{
						DepthStencil = InputSceneTextures->GetContents()->SceneDepthTexture->GetRHI();
					}

					FCompushadySceneTextures SceneTextures = {};
					FillSceneTextures(SceneTextures, RHICmdList, RenderTarget, InputSceneTextures->GetContents());

					const FVector2D ScreenSize = FVector2D(RenderTarget->GetDesc().GetSize().X, RenderTarget->GetDesc().GetSize().Y);

					Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
						RHICmdList, VertexShaderRef, PixelShaderRef, RenderTarget, DepthStencil, [&]()
						{
							FillRasterizerConfig(View.ViewMatrices, ScreenSize);
							Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings);
							Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures);
							RHICmdList.DrawPrimitive(0, NumVertices / 3, NumInstances);
						});
				});
		}
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

bool UCompushadyBlendable::InitFromHLSLAdvanced(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages)
{
	PostProcessLocation = InPostProcessLocation;
	VertexShaderRef = Compushady::Utils::CreateVertexShaderFromHLSL(VertexShaderCode, VertexShaderEntryPoint, VSResourceBindings, ErrorMessages);
	if (!VertexShaderRef.IsValid())
	{
		return false;
	}

	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromHLSL(PixelShaderCode, PixelShaderEntryPoint, PSResourceBindings, ErrorMessages);
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

bool UCompushadyBlendable::UpdateResourcesAdvanced(const FCompushadyResourceArray& InVSResourceArray, const FCompushadyResourceArray& InPSResourceArray, const int32 InNumVertices, const int32 InNumInstances, const FCompushadyBlendableRasterizerConfig& InBlendableRasterizerConfig, FString& ErrorMessages)
{
	if (InNumVertices <= 0 || InNumInstances <= 0)
	{
		return false;
	}

	if (!Compushady::Utils::ValidateResourceBindings(InVSResourceArray, VSResourceBindings, ErrorMessages))
	{
		return false;
	}

	if (!Compushady::Utils::ValidateResourceBindings(InPSResourceArray, PSResourceBindings, ErrorMessages))
	{
		return false;
	}

	NumVertices = InNumVertices;
	NumInstances = InNumInstances;

	RasterizerConfig = InBlendableRasterizerConfig;

	UntrackResources();
	VSResourceArray = InVSResourceArray;
	PSResourceArray = InPSResourceArray;
	TrackResources(VSResourceArray);
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

bool UCompushadyBlendable::UpdateResourcesByMapAdvanced(const TMap<FString, TScriptInterface<ICompushadyBindable>>& InVSResourceMap, const TMap<FString, TScriptInterface<ICompushadyBindable>>& InPSResourceMap, const int32 InNumVertices, const int32 InNumInstances, const FCompushadyBlendableRasterizerConfig& InBlendableRasterizerConfig, FString& ErrorMessages)
{
	FCompushadyResourceArray InVSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(InVSResourceMap, VSResourceBindings, InVSResourceArray, ErrorMessages))
	{
		return false;

	}
	FCompushadyResourceArray InPSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(InPSResourceMap, PSResourceBindings, InPSResourceArray, ErrorMessages))
	{
		return false;
	}

	return UpdateResourcesAdvanced(InVSResourceArray, InPSResourceArray, InNumVertices, InNumInstances, InBlendableRasterizerConfig, ErrorMessages);
}

FPixelShaderRHIRef UCompushadyBlendable::GetPixelShader() const
{
	return PixelShaderRef;
}

FGuid UCompushadyBlendable::AddToBlitter(UObject* WorldContextObject, const int32 Priority)
{
#if COMPUSHADY_UE_VERSION >= 53
	TSharedPtr<FCompushadyPostProcess, ESPMode::ThreadSafe> NewViewExtension = FSceneViewExtensions::NewExtension<FCompushadyPostProcess>(VertexShaderRef, VSResourceBindings, VSResourceArray, PixelShaderRef, PSResourceBindings, PSResourceArray, PostProcessLocation, NumVertices, NumInstances, RasterizerConfig);
	FGuid Guid = WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddViewExtension(NewViewExtension, this);
	NewViewExtension->SetPriority(Priority);
	return Guid;
#else
	return FGuid::NewGuid();
#endif
}