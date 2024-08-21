// Copyright 2023 - Roberto De Ioris.


#include "CompushadyBlendable.h"

#if COMPUSHADY_UE_VERSION >= 53
#include "PostProcess/PostProcessMaterialInputs.h"
#include "PostProcess/DrawRectangle.h"
#include "SceneViewExtension.h"
#include "SystemTextures.h"
#include "FXRenderingUtils.h"

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

	FIntRect GetViewRectAndFillCBVZero(const FSceneView& View, const bool bBeforeUpscaling)
	{
		auto GetScreenSize = [&]()
			{
				FIntRect CurrentViewRect = View.UnconstrainedViewRect;
				if (bBeforeUpscaling && View.bIsViewInfo)
				{
					CurrentViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
				}
				return CurrentViewRect;
			};

		FIntRect ScreenSize = GetScreenSize();

		if (CBVData.Num() == 0)
		{
			return ScreenSize;
		}

		auto SetMatrixByOffset = [&](const int32 Offset, const FMatrix& Matrix)
			{
				if (Offset >= 0 && static_cast<int64>(Offset + (sizeof(float) * 16)) <= CBVData.Num())
				{
					FMemory::Memcpy(CBVData.GetData() + Offset, FMatrix44f(Matrix).M, sizeof(float) * 16);
				}
			};

		auto SetVector2ByOffset = [&](const int32 Offset, const FVector2D Vector)
			{
				if (Offset >= 0 && static_cast<int64>(Offset + (sizeof(float) * 2)) <= CBVData.Num())
				{
					*(reinterpret_cast<float*>(CBVData.GetData() + Offset)) = Vector.X;
					*(reinterpret_cast<float*>(CBVData.GetData() + Offset + sizeof(float))) = Vector.Y;
				}
			};

		auto SetVector4ByOffset = [&](const int32 Offset, const FVector4 Vector)
			{
				if (Offset >= 0 && static_cast<int64>(Offset + (sizeof(float) * 4)) <= CBVData.Num())
				{
					*(reinterpret_cast<float*>(CBVData.GetData() + Offset)) = Vector.X;
					*(reinterpret_cast<float*>(CBVData.GetData() + Offset + sizeof(float))) = Vector.Y;
					*(reinterpret_cast<float*>(CBVData.GetData() + Offset + sizeof(float) * 2)) = Vector.Z;
					*(reinterpret_cast<float*>(CBVData.GetData() + Offset + sizeof(float) * 3)) = Vector.W;
				}
			};

		auto SetFloatByOffset = [&](const int32 Offset, const float Value)
			{
				if (Offset >= 0 && static_cast<int64>(Offset + sizeof(float)) <= CBVData.Num())
				{
					*(reinterpret_cast<float*>(CBVData.GetData() + Offset)) = Value;
				}
			};

		SetMatrixByOffset(RasterizerConfig.MatricesConfig.ViewMatrixOffset, View.ViewMatrices.GetViewMatrix());
		SetMatrixByOffset(RasterizerConfig.MatricesConfig.ProjectionMatrixOffset, View.ViewMatrices.GetProjectionMatrix());
		SetMatrixByOffset(RasterizerConfig.MatricesConfig.InverseViewMatrixOffset, View.ViewMatrices.GetInvViewMatrix());
		SetMatrixByOffset(RasterizerConfig.MatricesConfig.InverseProjectionMatrixOffset, View.ViewMatrices.GetInvProjectionMatrix());
		SetVector2ByOffset(RasterizerConfig.MatricesConfig.ScreenSizeFloat2Offset, ScreenSize.Size());

		SetMatrixByOffset(RasterizerConfig.MatricesConfig.ViewProjectionMatrixOffset, View.ViewMatrices.GetViewProjectionMatrix());
		SetMatrixByOffset(RasterizerConfig.MatricesConfig.InverseViewProjectionMatrixOffset, View.ViewMatrices.GetInvViewProjectionMatrix());

		SetVector4ByOffset(RasterizerConfig.MatricesConfig.ViewOriginFloat4Offset, View.ViewMatrices.GetViewOrigin());

		SetFloatByOffset(RasterizerConfig.MatricesConfig.DeltaTimeFloatOffset, View.Family->Time.GetDeltaRealTimeSeconds());
		SetFloatByOffset(RasterizerConfig.MatricesConfig.TimeFloatOffset, View.Family->Time.GetRealTimeSeconds());

		return ScreenSize;
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

		const FIntRect ViewRect = GetViewRectAndFillCBVZero(View, false);

		if (PixelShaderRef)
		{
			if (!VertexShaderRef)
			{
				FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
				TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);

				GraphBuilder.AddPass(
					RDG_EVENT_NAME("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
					ERDGPassFlags::None,
					[this, ViewRect, VertexShader, Output, SceneColorInput, InOutInputs](FRHICommandList& RHICmdList)
					{
						FTextureRHIRef RenderTarget = Output.Texture->GetRHI();

						FCompushadySceneTextures SceneTextures = {};
						FillSceneTextures(SceneTextures, RHICmdList, SceneColorInput.Texture->GetRHI(), InOutInputs.SceneTextures.SceneTextures->GetContents());

						RasterizerConfig.RasterizerConfig.BlendMode = ECompushadyRasterizerBlendMode::Always;

						Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
							RHICmdList, VertexShader.GetVertexShader(), PixelShaderRef, nullptr, RenderTarget, [&]()
							{
								Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures, true);
								UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, ViewRect.Min.X, ViewRect.Min.Y, ViewRect.Width(), ViewRect.Height(),
									0, 0, 1, 1,
									ViewRect.Size(),
									FIntPoint(1, 1),
									INDEX_NONE,
									false, EDRF_UseTriangleOptimization);
							}, RasterizerConfig.RasterizerConfig);
					});
			}
			else
			{
				const FSceneTextureUniformParameters* SceneTextureUniform = InOutInputs.SceneTextures.SceneTextures->GetContents();

				GraphBuilder.AddPass(
					RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
					ERDGPassFlags::None,
					[this, ViewRect, SceneColorInput, Output, SceneTextureUniform, CopyBufferData = CBVData](FRHICommandList& RHICmdList)
					{
						FCompushadySceneTextures SceneTextures = {};
						FillSceneTextures(SceneTextures, RHICmdList, SceneColorInput.Texture->GetRHI(), SceneTextureUniform);

						FTextureRHIRef RenderTarget = Output.Texture->GetRHI();

						FTextureRHIRef DepthStencil = SceneTextureUniform->SceneDepthTexture->GetRHI();

						Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
							RHICmdList, VertexShaderRef, PixelShaderRef, &ViewRect, RenderTarget, DepthStencil, [&]()
							{
								if (VSResourceArray.CBVs.IsValidIndex(0) && CopyBufferData.Num() > 0)
								{
									VSResourceArray.CBVs[0]->SyncBufferDataWithData(RHICmdList, CopyBufferData);
								}
								Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings, false);
								Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures, false);
								Compushady::Utils::DrawVertices(RHICmdList, NumVertices, NumInstances, RasterizerConfig.RasterizerConfig);
							}, RasterizerConfig.RasterizerConfig);
					});
			}
		}
		else if (ComputeShaderRef)
		{
			const FSceneTextureUniformParameters* SceneTextureUniform = InOutInputs.SceneTextures.SceneTextures->GetContents();

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
				ERDGPassFlags::None,
				[this, SceneTextureUniform, &View, CopyBufferData = CBVData](FRHICommandList& RHICmdList)
				{
					FTextureRHIRef RenderTarget = SceneTextureUniform->SceneColorTexture->GetRHI();
					FCompushadySceneTextures SceneTextures = {};
					FillSceneTextures(SceneTextures, RHICmdList, RenderTarget, SceneTextureUniform);

					SetComputePipelineState(RHICmdList, ComputeShaderRef);

					if (ComputeResourceArray.CBVs.IsValidIndex(0) && CopyBufferData.Num() > 0)
					{
						ComputeResourceArray.CBVs[0]->SyncBufferDataWithData(RHICmdList, CopyBufferData);
					}
					Compushady::Utils::SetupPipelineParameters(RHICmdList, ComputeShaderRef, ComputeResourceArray, ComputeResourceBindings, SceneTextures, false);

					RHICmdList.DispatchComputeShader(XYZ.X, XYZ.Y, XYZ.Z);
				});

			// ensure to return the original texture!
			return SceneColorInput;
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

	FComputeShaderRHIRef ComputeShaderRef = nullptr;
	FCompushadyResourceBindings ComputeResourceBindings;
	FCompushadyResourceArray ComputeResourceArray;

	FIntVector XYZ;

	int32 CompushadyPriority = 0;

	FCompushadyBlendableRasterizerConfig RasterizerConfig;

	TArray<uint8> CBVData;
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
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override
	{
		// make a copy of the CBV for thread-safe management (we need to rely on copies)
		if (VertexShaderRef && VSResourceArray.CBVs.Num() > 0)
		{
			CBVData = VSResourceArray.CBVs[0]->GetBufferData();
		}
		else if (ComputeShaderRef && ComputeResourceArray.CBVs.Num() > 0)
		{
			CBVData = ComputeResourceArray.CBVs[0]->GetBufferData();
		}
	}

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

		const FIntRect ViewRect = GetViewRectAndFillCBVZero(View, true);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
			ERDGPassFlags::None,
			[this, ViewRect, VertexShader, InputSceneTextures](FRHICommandList& RHICmdList)
			{
				FTextureRHIRef RenderTarget = InputSceneTextures->GetContents()->SceneColorTexture->GetRHI();

				FCompushadySceneTextures SceneTextures = {};
				FillSceneTextures(SceneTextures, RHICmdList, RenderTarget, InputSceneTextures->GetContents());

				RasterizerConfig.RasterizerConfig.BlendMode = ECompushadyRasterizerBlendMode::Always;

				Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
					RHICmdList, VertexShader.GetVertexShader(), PixelShaderRef, nullptr, RenderTarget, [&]()
					{
						Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures, true);
						UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, ViewRect.Min.X, ViewRect.Min.Y, ViewRect.Width(), ViewRect.Height(),
							0, 0, 1, 1,
							ViewRect.Size(),
							FIntPoint(1, 1),
							INDEX_NONE,
							false, EDRF_UseTriangleOptimization);
					}, RasterizerConfig.RasterizerConfig);
			});
	}

	virtual int32 GetPriority() const override { return CompushadyPriority; }

protected:
	EPostProcessingPass RequiredPass = EPostProcessingPass::Tonemap;
	bool bPrePostProcess = false;
};

class FCompushadyPostProcess : public FSceneViewExtensionBase, public ICompushadyViewExtension, public ICompushadyTransientBlendable
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
		case ECompushadyPostProcessLocation::AfterBasePass:
			bAfterBasePass = true;
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

	FCompushadyPostProcess(const FAutoRegister& AutoRegister, FComputeShaderRHIRef InComputeShaderRef, const FCompushadyResourceBindings& InComputeResourceBindings, const FCompushadyResourceArray& InComputeResourceArray, const ECompushadyPostProcessLocation PostProcessLocation, const FIntVector& InXYZ, const FCompushadyBlendableRasterizerConfig& InRasterizerConfig) :
		FCompushadyPostProcess(AutoRegister, nullptr, {}, {}, nullptr, {}, {}, PostProcessLocation, 0, 0, InRasterizerConfig)
	{
		ComputeShaderRef = InComputeShaderRef;
		ComputeResourceBindings = InComputeResourceBindings;
		ComputeResourceArray = InComputeResourceArray;
		XYZ = InXYZ;
	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override
	{
		// make a copy of the CBV for thread-safe management (we need to rely on copies)
		if (VertexShaderRef && VSResourceArray.CBVs.Num() > 0)
		{
			CBVData = VSResourceArray.CBVs[0]->GetBufferData();
		}
		else if (ComputeShaderRef && ComputeResourceArray.CBVs.Num() > 0)
		{
			CBVData = ComputeResourceArray.CBVs[0]->GetBufferData();
		}
	}

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
	{
		if (!bPrePostProcess && !bAfterBasePass && Pass == RequiredPass && bIsPassEnabled)
		{
			InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateSP(this, &FCompushadyPostProcess::PostProcessCallback_RenderThread));
		}
	}

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
	{
		if (!bAfterBasePass)
		{
			return;
		}

		const FIntRect ViewRect = GetViewRectAndFillCBVZero(InView, true);

		if (PixelShaderRef)
		{
			if (!VertexShaderRef)
			{
				FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(InView.GetFeatureLevel());
				TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);

				GraphBuilder.AddPass(
					RDG_EVENT_NAME("FCompushadyPostProcess::PostRenderBasePassDeferred_RenderThread"),
					ERDGPassFlags::None,
					[this, ViewRect, VertexShader, SceneTextures, RenderTargets](FRHICommandList& RHICmdList)
					{
						FCompushadySceneTextures CompushadySceneTextures = {};
						FillSceneTextures(CompushadySceneTextures, RHICmdList, SceneTextures->GetContents()->SceneColorTexture->GetRHI(), SceneTextures->GetContents());

						TArray<FTextureRHIRef> RTVs;
						for (int32 RTVIndex = 0; RTVIndex < MaxSimultaneousRenderTargets; RTVIndex++)
						{
							if (RenderTargets[RTVIndex].GetTexture() != nullptr)
							{
								RTVs.Add(RenderTargets[RTVIndex].GetTexture()->GetRHI());
								switch (RTVIndex)
								{
								case 0:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::SceneColor, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 1:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferA, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 2:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferB, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 3:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferC, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 4:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferD, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 5:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferE, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								}
							}
						}

						if (RTVs.Num() == 0)
						{
							return;
						}

						RasterizerConfig.RasterizerConfig.BlendMode = ECompushadyRasterizerBlendMode::Always;

						Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostRenderBasePassDeferred_RenderThread"),
							RHICmdList, VertexShader.GetVertexShader(), PixelShaderRef, nullptr, RTVs, nullptr, [&]()
							{
								Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, CompushadySceneTextures, true);
								UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, ViewRect.Min.X, ViewRect.Min.Y, ViewRect.Width(), ViewRect.Height(),
									0, 0, 1, 1,
									ViewRect.Size(),
									FIntPoint(1, 1),
									INDEX_NONE,
									false, EDRF_UseTriangleOptimization);
							}, RasterizerConfig.RasterizerConfig);
					});
			}
			else
			{
				GraphBuilder.AddPass(
					RDG_EVENT_NAME("FCompushadyPostProcess::PostRenderBasePassDeferred_RenderThread"),
					ERDGPassFlags::None,
					[this, ViewRect, SceneTextures, RenderTargets, CopyBufferData = CBVData](FRHICommandList& RHICmdList)
					{
						FTextureRHIRef DepthStencil = SceneTextures->GetContents()->SceneDepthTexture->GetRHI();

						FCompushadySceneTextures CompushadySceneTextures = {};
						FillSceneTextures(CompushadySceneTextures, RHICmdList, SceneTextures->GetContents()->SceneColorTexture->GetRHI(), SceneTextures->GetContents());

						TArray<FTextureRHIRef> RTVs;
						for (int32 RTVIndex = 0; RTVIndex < MaxSimultaneousRenderTargets; RTVIndex++)
						{
							if (RenderTargets[RTVIndex].GetTexture() != nullptr)
							{
								RTVs.Add(RenderTargets[RTVIndex].GetTexture()->GetRHI());
								switch (RTVIndex)
								{
								case 0:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::SceneColor, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 1:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferA, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 2:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferB, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 3:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferC, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 4:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferD, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								case 5:
									CompushadySceneTextures.SetTexture(ECompushadySceneTexture::GBufferE, RenderTargets[RTVIndex].GetTexture()->GetRHI());
									break;
								}
							}
						}

						if (RTVs.Num() == 0)
						{
							return;
						}

						Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostRenderBasePassDeferred_RenderThread"),
							RHICmdList, VertexShaderRef, PixelShaderRef, &ViewRect, RTVs, DepthStencil, [&]()
							{
								if (VSResourceArray.CBVs.IsValidIndex(0) && CopyBufferData.Num() > 0)
								{
									VSResourceArray.CBVs[0]->SyncBufferDataWithData(RHICmdList, CopyBufferData);
								}
								Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings, false);
								Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, CompushadySceneTextures, false);
								Compushady::Utils::DrawVertices(RHICmdList, NumVertices, NumInstances, RasterizerConfig.RasterizerConfig);
							}, RasterizerConfig.RasterizerConfig);
					});
			}
		}
		else if (ComputeShaderRef)
		{
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("FCompushadyPostProcess::PostRenderBasePassDeferred_RenderThread"),
				ERDGPassFlags::None,
				[this, SceneTextures, &InView, CopyBufferData = CBVData](FRHICommandList& RHICmdList)
				{
					FTextureRHIRef RenderTarget = SceneTextures->GetContents()->SceneColorTexture->GetRHI();
					FCompushadySceneTextures CompushadySceneTextures = {};
					FillSceneTextures(CompushadySceneTextures, RHICmdList, RenderTarget, SceneTextures->GetContents());

					SetComputePipelineState(RHICmdList, ComputeShaderRef);

					if (ComputeResourceArray.CBVs.IsValidIndex(0) && CopyBufferData.Num() > 0)
					{
						ComputeResourceArray.CBVs[0]->SyncBufferDataWithData(RHICmdList, CopyBufferData);
					}
					Compushady::Utils::SetupPipelineParameters(RHICmdList, ComputeShaderRef, ComputeResourceArray, ComputeResourceBindings, CompushadySceneTextures, false);

					RHICmdList.DispatchComputeShader(XYZ.X, XYZ.Y, XYZ.Z);
				});
		}
	}

	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override
	{
		if (!bPrePostProcess)
		{
			return;
		}

		TRDGUniformBufferRef<FSceneTextureUniformParameters> InputSceneTextures = *((TRDGUniformBufferRef<FSceneTextureUniformParameters>*) & Inputs);

		const FIntRect ViewRect = GetViewRectAndFillCBVZero(View, true);

		if (PixelShaderRef)
		{
			if (!VertexShaderRef)
			{
				FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
				TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);

				GraphBuilder.AddPass(
					RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
					ERDGPassFlags::None,
					[this, ViewRect, VertexShader, InputSceneTextures](FRHICommandList& RHICmdList)
					{
						FTexture2DRHIRef RenderTarget = InputSceneTextures->GetContents()->SceneColorTexture->GetRHI();

						FCompushadySceneTextures SceneTextures = {};
						FillSceneTextures(SceneTextures, RHICmdList, InputSceneTextures->GetContents()->SceneColorTexture->GetRHI(), InputSceneTextures->GetContents());

						RasterizerConfig.RasterizerConfig.BlendMode = ECompushadyRasterizerBlendMode::Always;

						Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
							RHICmdList, VertexShader.GetVertexShader(), PixelShaderRef, nullptr, RenderTarget, [&]()
							{
								Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures, true);
								UE::Renderer::PostProcess::DrawPostProcessPass(RHICmdList, VertexShader, ViewRect.Min.X, ViewRect.Min.Y, ViewRect.Width(), ViewRect.Height(),
									0, 0, 1, 1,
									ViewRect.Size(),
									FIntPoint(1, 1),
									INDEX_NONE,
									false, EDRF_UseTriangleOptimization);
							}, RasterizerConfig.RasterizerConfig);
					});
			}
			else
			{
				GraphBuilder.AddPass(
					RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
					ERDGPassFlags::None,
					[this, ViewRect, InputSceneTextures, CopyBufferData = CBVData](FRHICommandList& RHICmdList)
					{
						FTextureRHIRef RenderTarget = InputSceneTextures->GetContents()->SceneColorTexture->GetRHI();
						FTextureRHIRef DepthStencil = InputSceneTextures->GetContents()->SceneDepthTexture->GetRHI();

						FCompushadySceneTextures SceneTextures = {};
						FillSceneTextures(SceneTextures, RHICmdList, RenderTarget, InputSceneTextures->GetContents());

						Compushady::Utils::RasterizeSimplePass_RenderThread(TEXT("FCompushadyPostProcess::PostProcessCallback_RenderThread"),
							RHICmdList, VertexShaderRef, PixelShaderRef, &ViewRect, RenderTarget, DepthStencil, [&]()
							{
								if (VSResourceArray.CBVs.IsValidIndex(0) && CopyBufferData.Num() > 0)
								{
									VSResourceArray.CBVs[0]->SyncBufferDataWithData(RHICmdList, CopyBufferData);
								}
								Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings, false);
								Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, SceneTextures, false);
								Compushady::Utils::DrawVertices(RHICmdList, NumVertices, NumInstances, RasterizerConfig.RasterizerConfig);
							}, RasterizerConfig.RasterizerConfig);
					});
			}
		}
		else if (ComputeShaderRef)
		{
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("FCompushadyPostProcess::PrePostProcessPass_RenderThread"),
				ERDGPassFlags::None,
				[this, InputSceneTextures, &View, CopyBufferData = CBVData](FRHICommandList& RHICmdList)
				{
					FTextureRHIRef RenderTarget = InputSceneTextures->GetContents()->SceneColorTexture->GetRHI();
					FCompushadySceneTextures SceneTextures = {};
					FillSceneTextures(SceneTextures, RHICmdList, RenderTarget, InputSceneTextures->GetContents());

					SetComputePipelineState(RHICmdList, ComputeShaderRef);

					if (ComputeResourceArray.CBVs.IsValidIndex(0) && CopyBufferData.Num() > 0)
					{
						ComputeResourceArray.CBVs[0]->SyncBufferDataWithData(RHICmdList, CopyBufferData);
					}
					Compushady::Utils::SetupPipelineParameters(RHICmdList, ComputeShaderRef, ComputeResourceArray, ComputeResourceBindings, SceneTextures, false);

					RHICmdList.DispatchComputeShader(XYZ.X, XYZ.Y, XYZ.Z);
				});
		}

	}

	virtual int32 GetPriority() const override { return CompushadyPriority; }

	void Disable() override
	{
		bEnabled = false;
	}

	bool IsEnabled() const
	{
		return bEnabled;
	}

protected:
	EPostProcessingPass RequiredPass = EPostProcessingPass::Tonemap;
	bool bPrePostProcess = false;
	bool bAfterBasePass = false;;

	bool bEnabled = true;
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

bool UCompushadyBlendable::InitFromHLSLCompute(const TArray<uint8>& ShaderCode, const FString& ShaderEntryPoint, const FString& TargetProfile, const ECompushadyPostProcessLocation InPostProcessLocation, FString& ErrorMessages)
{
	PostProcessLocation = InPostProcessLocation;
	ComputeShaderRef = Compushady::Utils::CreateComputeShaderFromHLSL(ShaderCode, ShaderEntryPoint, ComputeResourceBindings, ThreadGroupSize, ErrorMessages, TargetProfile);
	return ComputeShaderRef.IsValid();
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
		ErrorMessages = "Invalid number of Vertices and Instances";
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

bool UCompushadyBlendable::UpdateComputeResourcesAdvanced(const FCompushadyResourceArray& InResourceArray, const FIntVector& InXYZ, const FCompushadyBlendableMatricesConfig& BlendableMatricesConfig, FString& ErrorMessages)
{
	if (InXYZ.X <= 0 || InXYZ.Y <= 0 || InXYZ.Z <= 0)
	{
		ErrorMessages = FString::Printf(TEXT("Invalid ThreadGroupCount %s"), *InXYZ.ToString());
		return false;
	}

	if (!Compushady::Utils::ValidateResourceBindings(InResourceArray, ComputeResourceBindings, ErrorMessages))
	{
		return false;
	}

	XYZ = InXYZ;

	RasterizerConfig.MatricesConfig = BlendableMatricesConfig;

	UntrackResources();
	ComputeResourceArray = InResourceArray;
	TrackResources(ComputeResourceArray);

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

bool UCompushadyBlendable::UpdateComputeResourcesByMapAdvanced(const TMap<FString, TScriptInterface<ICompushadyBindable>>& InResourceMap, const FIntVector& InXYZ, const FCompushadyBlendableMatricesConfig& BlendableMatricesConfig, FString& ErrorMessages)
{
	FCompushadyResourceArray InResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(InResourceMap, ComputeResourceBindings, InResourceArray, ErrorMessages))
	{
		return false;

	}

	return UpdateComputeResourcesAdvanced(InResourceArray, InXYZ, BlendableMatricesConfig, ErrorMessages);
}

FPixelShaderRHIRef UCompushadyBlendable::GetPixelShader() const
{
	return PixelShaderRef;
}

FGuid UCompushadyBlendable::AddToBlitter(UObject* WorldContextObject, const int32 Priority)
{
#if COMPUSHADY_UE_VERSION >= 53
	TSharedPtr<FCompushadyPostProcess, ESPMode::ThreadSafe> NewViewExtension = nullptr;

	if (PixelShaderRef)
	{
		NewViewExtension = FSceneViewExtensions::NewExtension<FCompushadyPostProcess>(VertexShaderRef, VSResourceBindings, VSResourceArray, PixelShaderRef, PSResourceBindings, PSResourceArray, PostProcessLocation, NumVertices, NumInstances, RasterizerConfig);
	}
	else if (ComputeShaderRef)
	{
		NewViewExtension = FSceneViewExtensions::NewExtension<FCompushadyPostProcess>(ComputeShaderRef, ComputeResourceBindings, ComputeResourceArray, PostProcessLocation, XYZ, RasterizerConfig);
	}
	else
	{
		return FGuid();
	}

	FSceneViewExtensionIsActiveFunctor PPActiveFunctor;

	PPActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* InSceneViewExtension, const FSceneViewExtensionContext& InContext)
		{
			const FCompushadyPostProcess* PostProces = reinterpret_cast<const FCompushadyPostProcess*>(InSceneViewExtension);
			return PostProces->IsEnabled();
		};

	NewViewExtension->IsActiveThisFrameFunctions.Add(MoveTemp(PPActiveFunctor));

	FGuid Guid = WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddViewExtension(NewViewExtension, this);
	NewViewExtension->SetPriority(Priority);
	return Guid;
#else
	return FGuid::NewGuid();
#endif
}