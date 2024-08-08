// Copyright 2023 - Roberto De Ioris.


#include "CompushadyRasterizer.h"
#include "CommonRenderResources.h"
#include "Compushady.h"
#include "Serialization/ArrayWriter.h"

bool UCompushadyRasterizer::InitVSPSFromHLSL(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	VertexShaderRef = Compushady::Utils::CreateVertexShaderFromHLSL(VertexShaderCode, VertexShaderEntryPoint, VSResourceBindings, ErrorMessages);
	if (!VertexShaderRef)
	{
		return false;
	}

	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromHLSL(PixelShaderCode, PixelShaderEntryPoint, PSResourceBindings, ErrorMessages);
	if (!PixelShaderRef)
	{
		return false;
	}

	return CreateVSPSRasterizerPipeline(RasterizerConfig, ErrorMessages);
}

bool UCompushadyRasterizer::InitMSPSFromHLSL(const TArray<uint8>& MeshShaderCode, const FString& MeshShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	FIntVector ThreadGroupSize;
	MeshShaderRef = Compushady::Utils::CreateMeshShaderFromHLSL(MeshShaderCode, MeshShaderEntryPoint, MSResourceBindings, ThreadGroupSize, ErrorMessages);
	if (!MeshShaderRef)
	{
		return false;
	}

	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromHLSL(PixelShaderCode, PixelShaderEntryPoint, PSResourceBindings, ErrorMessages);
	if (!PixelShaderRef)
	{
		return false;
	}

	return CreateMSPSRasterizerPipeline(RasterizerConfig, ErrorMessages);
}

bool UCompushadyRasterizer::InitVSPSFromGLSL(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	VertexShaderRef = Compushady::Utils::CreateVertexShaderFromGLSL(VertexShaderCode, VertexShaderEntryPoint, VSResourceBindings, ErrorMessages);
	if (!VertexShaderRef)
	{
		return false;
	}

	PixelShaderRef = Compushady::Utils::CreatePixelShaderFromGLSL(PixelShaderCode, PixelShaderEntryPoint, PSResourceBindings, ErrorMessages);
	if (!VertexShaderRef)
	{
		return false;
	}

	return CreateVSPSRasterizerPipeline(RasterizerConfig, ErrorMessages);
}

bool UCompushadyRasterizer::CreateVSPSRasterizerPipeline(const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	// check for semantics
	if (VSResourceBindings.InputSemantics.Num() > 0)
	{
		ErrorMessages = FString::Printf(TEXT("Unsupported input semantic in vertex shader: %s/%d"), *(VSResourceBindings.InputSemantics[0]).Name, VSResourceBindings.InputSemantics[0].Index);
		return false;
	}

	for (const Compushady::FCompushadyShaderSemantic& Semantic : PSResourceBindings.InputSemantics)
	{
		if (!VSResourceBindings.OutputSemantics.Contains(Semantic))
		{
			ErrorMessages = FString::Printf(TEXT("Unknown/Unaligned input semantic in pixel shader: %s/%d (register: %u mask: 0x%x)"), *Semantic.Name, Semantic.Index, Semantic.Register, Semantic.Mask);
			return false;
		}
	}

	FillPipelineStateInitializer(RasterizerConfig);

	PipelineStateInitializer.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
	PipelineStateInitializer.BoundShaderState.VertexShaderRHI = VertexShaderRef;
	PipelineStateInitializer.BoundShaderState.PixelShaderRHI = PixelShaderRef;

	return true;
}

void UCompushadyRasterizer::FillPipelineStateInitializer(const FCompushadyRasterizerConfig& RasterizerConfig)
{
	Compushady::Utils::FillRasterizerPipelineStateInitializer(RasterizerConfig, PipelineStateInitializer);

	PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<true, CF_LessEqual, true, CF_Always, SO_Keep, SO_Keep, SO_Replace, true, CF_Always, SO_Keep, SO_Keep, SO_Replace>::GetRHI();
	PipelineStateInitializer.BlendState = TStaticBlendState<>::GetRHI();

	static int32 Denominators[] = { 3, 2, 1 };
	DrawDenominator = Denominators[static_cast<int32>(RasterizerConfig.PrimitiveType)];
}

bool UCompushadyRasterizer::CreateMSPSRasterizerPipeline(const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	// check for semantics
	if (MSResourceBindings.InputSemantics.Num() > 0)
	{
		ErrorMessages = FString::Printf(TEXT("Unsupported input semantic in mesh shader: %s/%d"), *(MSResourceBindings.InputSemantics[0]).Name, MSResourceBindings.InputSemantics[0].Index);
		return false;
	}

	for (const Compushady::FCompushadyShaderSemantic& Semantic : PSResourceBindings.InputSemantics)
	{
		if (!VSResourceBindings.OutputSemantics.Contains(Semantic))
		{
			ErrorMessages = FString::Printf(TEXT("Unknown/Unaligned input semantic in pixel shader: %s/%d (register: %u mask: 0x%x)"), *Semantic.Name, Semantic.Index, Semantic.Register, Semantic.Mask);
			return false;
		}
	}

	FillPipelineStateInitializer(RasterizerConfig);

	PipelineStateInitializer.BoundShaderState.VertexDeclarationRHI = nullptr;
	PipelineStateInitializer.BoundShaderState.SetMeshShader(MeshShaderRef);
	PipelineStateInitializer.BoundShaderState.PixelShaderRHI = PixelShaderRef;

	return true;
}

void UCompushadyRasterizer::DrawByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled)
{
	FString ErrorMessages;

	FCompushadyResourceArray VSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(VSResourceMap, VSResourceBindings, VSResourceArray, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	FCompushadyResourceArray PSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(PSResourceMap, PSResourceBindings, PSResourceArray, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	return Draw(VSResourceArray, PSResourceArray, RTVs, DSV, NumVertices, NumInstances, RasterizeConfig, OnSignaled);
}

void UCompushadyRasterizer::Draw(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Rasterizer is already running");
		return;
	}

	if (NumVertices <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid number of vertices %d"), NumVertices));
		return;
	}

	if (NumInstances <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid number of instances %d"), NumInstances));
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(VSResourceArray, VSResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	if (!Compushady::Utils::ValidateResourceBindings(PSResourceArray, PSResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	if (!SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture))
	{
		OnSignaled.ExecuteIfBound(false, "Invalid RTVs");
		return;
	}

	TrackResources(VSResourceArray);
	TrackResources(PSResourceArray);

	EnqueueToGPU(
		[this, NumVertices, NumInstances, VSResourceArray, PSResourceArray, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, RasterizeConfig](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;

			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::Draw"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Load_Store, EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil, Width, Height))
			{
				SetupRasterization_RenderThread(RHICmdList, RasterizeConfig, Width, Height);

				Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings, true);
				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, {}, true);

				RHICmdList.DrawPrimitive(0, NumVertices / DrawDenominator, NumInstances);

				RHICmdList.EndRenderPass();
			}
		}, OnSignaled);
}

void UCompushadyRasterizer::ClearAndDraw(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Rasterizer is already running");
		return;
	}

	if (NumVertices <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid number of vertices %d"), NumVertices));
		return;
	}

	if (NumInstances <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid number of instances %d"), NumInstances));
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(VSResourceArray, VSResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	if (!Compushady::Utils::ValidateResourceBindings(PSResourceArray, PSResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	if (!SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture))
	{
		OnSignaled.ExecuteIfBound(false, "Invalid RTVs");
		return;
	}

	TrackResources(VSResourceArray);
	TrackResources(PSResourceArray);

	EnqueueToGPU(
		[this, NumVertices, NumInstances, VSResourceArray, PSResourceArray, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, RasterizeConfig](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;

			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::ClearAndDraw"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Clear_Store, EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil, Width, Height))
			{
				SetupRasterization_RenderThread(RHICmdList, RasterizeConfig, Width, Height);

				Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings, true);
				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, {}, true);

				RHICmdList.DrawPrimitive(0, NumVertices / DrawDenominator, NumInstances);

				RHICmdList.EndRenderPass();
			}
		}, OnSignaled);
}

bool UCompushadyRasterizer::ClearAndDrawSync(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, FString& ErrorMessages)
{
	if (IsRunning())
	{
		ErrorMessages = "The Rasterizer is already running";
		return false;
	}

	if (NumVertices <= 0)
	{
		ErrorMessages = FString::Printf(TEXT("Invalid number of vertices %d"), NumVertices);
		return false;
	}

	if (NumInstances <= 0)
	{
		ErrorMessages = FString::Printf(TEXT("Invalid number of instances %d"), NumInstances);
		return false;
	}

	if (!Compushady::Utils::ValidateResourceBindings(VSResourceArray, VSResourceBindings, ErrorMessages))
	{
		return false;
	}

	if (!Compushady::Utils::ValidateResourceBindings(PSResourceArray, PSResourceBindings, ErrorMessages))
	{
		return false;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	if (!SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture))
	{
		ErrorMessages = "Invalid RTVs";
		return false;
	}

	TrackResources(VSResourceArray);
	TrackResources(PSResourceArray);

	EnqueueToGPUSync(
		[this, NumVertices, NumInstances, VSResourceArray, PSResourceArray, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, RasterizeConfig](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;

			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::ClearAndDrawSync"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Clear_Store, EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil, Width, Height))
			{
				SetupRasterization_RenderThread(RHICmdList, RasterizeConfig, Width, Height);

				Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings, true);
				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, {}, true);

				RHICmdList.DrawPrimitive(0, NumVertices / DrawDenominator, NumInstances);

				RHICmdList.EndRenderPass();
			}
		});

	return true;
}

void UCompushadyRasterizer::ClearAndDrawByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled)
{
	FString ErrorMessages;

	FCompushadyResourceArray VSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(VSResourceMap, VSResourceBindings, VSResourceArray, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	FCompushadyResourceArray PSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(PSResourceMap, PSResourceBindings, PSResourceArray, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	return ClearAndDraw(VSResourceArray, PSResourceArray, RTVs, DSV, NumVertices, NumInstances, RasterizeConfig, OnSignaled);
}

bool UCompushadyRasterizer::ClearAndDrawByMapSync(const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const FCompushadyRasterizeConfig& RasterizeConfig, FString& ErrorMessages)
{
	FCompushadyResourceArray VSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(VSResourceMap, VSResourceBindings, VSResourceArray, ErrorMessages))
	{
		return false;
	}

	FCompushadyResourceArray PSResourceArray;
	if (!Compushady::Utils::ValidateResourceBindingsMap(PSResourceMap, PSResourceBindings, PSResourceArray, ErrorMessages))
	{
		return false;
	}

	return ClearAndDrawSync(VSResourceArray, PSResourceArray, RTVs, DSV, NumVertices, NumInstances, RasterizeConfig, ErrorMessages);
}

void UCompushadyRasterizer::SetupRasterization_RenderThread(FRHICommandListImmediate& RHICmdList, const FCompushadyRasterizeConfig& RasterizeConfig, const int32 Width, const int32 Height)
{
	float ViewportMinX = RasterizeConfig.Viewport.Min.X > 0 ? RasterizeConfig.Viewport.Min.X : 0;
	float ViewportMinY = RasterizeConfig.Viewport.Min.Y > 0 ? RasterizeConfig.Viewport.Min.Y : 0;
	float ViewportMinZ = RasterizeConfig.Viewport.Min.Z > 0 ? RasterizeConfig.Viewport.Min.Z : 0;
	float ViewportMaxX = RasterizeConfig.Viewport.Max.X > 0 ? RasterizeConfig.Viewport.Max.X : Width;
	float ViewportMaxY = RasterizeConfig.Viewport.Max.Y > 0 ? RasterizeConfig.Viewport.Max.Y : Height;
	float ViewportMaxZ = RasterizeConfig.Viewport.Max.Z > 0 ? RasterizeConfig.Viewport.Max.Z : 1.0;

	RHICmdList.SetViewport(ViewportMinX, ViewportMinY, ViewportMinZ, ViewportMaxX, ViewportMaxY, ViewportMaxZ);
	RHICmdList.SetScissorRect(RasterizeConfig.Scissor.GetArea() > 0, RasterizeConfig.Scissor.Min.X, RasterizeConfig.Scissor.Min.Y, RasterizeConfig.Scissor.Max.X, RasterizeConfig.Scissor.Max.Y);

	RHICmdList.ApplyCachedRenderTargets(PipelineStateInitializer);

	SetGraphicsPipelineState(RHICmdList, PipelineStateInitializer, RasterizeConfig.StencilValue);
}

void UCompushadyRasterizer::DrawIndirect(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, UCompushadyResource* CommandBuffer, const int32 Offset, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Rasterizer is already running");
		return;
	}

	if (!CommandBuffer)
	{
		OnSignaled.ExecuteIfBound(false, TEXT("CommandBuffer is NULL"));
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(VSResourceArray, VSResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	if (!Compushady::Utils::ValidateResourceBindings(PSResourceArray, PSResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	if (!SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture))
	{
		OnSignaled.ExecuteIfBound(false, "Invalid RTVs");
		return;
	}

	TrackResources(VSResourceArray);
	TrackResources(PSResourceArray);

	FRHIBuffer* RHICommandBuffer = CommandBuffer->GetBufferRHI();
	TrackResource(CommandBuffer);

	EnqueueToGPU(
		[this, VSResourceArray, PSResourceArray, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, RHICommandBuffer, Offset, RasterizeConfig](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;
			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::DrawIndirect"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Load_Store, EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil, Width, Height))
			{
				SetupRasterization_RenderThread(RHICmdList, RasterizeConfig, Width, Height);

				Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings, true);
				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, {}, true);

				RHICmdList.DrawPrimitiveIndirect(RHICommandBuffer, Offset);

				RHICmdList.EndRenderPass();
			}

		}, OnSignaled);
}

bool UCompushadyRasterizer::SetupRenderTargets(const TArray<UCompushadyRTV*>& RTVs, UCompushadyDSV* DSV, TStaticArray<FRHITexture*, 8>& RenderTargets, int32& RenderTargetsEnabled, FRHITexture*& DepthStencilTexture)
{
	for (int32 Index = 0; Index < RTVs.Num(); Index++)
	{
		if (!RTVs[Index])
		{
			return false;
		}
		RenderTargets[Index] = RTVs[Index]->GetTextureRHI();
		RenderTargetsEnabled++;
		TrackResource(RTVs[Index]);
	}

	if (DSV)
	{
		DepthStencilTexture = DSV->GetTextureRHI();
		TrackResource(DSV);
	}

	return true;
}

bool UCompushadyRasterizer::BeginRenderPass_RenderThread(const TCHAR* Name, FRHICommandListImmediate& RHICmdList, const TStaticArray<FRHITexture*, 8>& RenderTargets, const int32 RenderTargetsEnabled, FRHITexture* DepthStencilTexture, const ERenderTargetActions ColorAction, const EDepthStencilTargetActions DepthStencilAction, uint32& Width, uint32& Height)
{
	for (int32 RenderTargetIndex = 0; RenderTargetIndex < RenderTargetsEnabled; RenderTargetIndex++)
	{
		RHICmdList.Transition(FRHITransitionInfo(RenderTargets[RenderTargetIndex], ERHIAccess::Unknown, ERHIAccess::RTV));
	}

	if (DepthStencilTexture)
	{
		RHICmdList.Transition(FRHITransitionInfo(DepthStencilTexture, ERHIAccess::Unknown, ERHIAccess::DSVRead | ERHIAccess::DSVWrite));
	}

	if (RenderTargetsEnabled > 0 && DepthStencilTexture)
	{
		FRHIRenderPassInfo Info(RenderTargetsEnabled,
			const_cast<FRHITexture**>(RenderTargets.GetData()),
			ColorAction,
			DepthStencilTexture,
			DepthStencilAction,
			FExclusiveDepthStencil::DepthWrite_StencilWrite);
		Width = RenderTargets[0]->GetSizeX();
		Height = RenderTargets[0]->GetSizeY();
		RHICmdList.BeginRenderPass(Info, Name);
		return true;
	}
	else if (RenderTargetsEnabled > 0)
	{
		FRHIRenderPassInfo Info(RenderTargetsEnabled,
			const_cast<FRHITexture**>(RenderTargets.GetData()),
			ColorAction);
		Width = RenderTargets[0]->GetSizeX();
		Height = RenderTargets[0]->GetSizeY();
		RHICmdList.BeginRenderPass(Info, Name);
		return true;
	}
	else if (DepthStencilTexture)
	{
		FRHIRenderPassInfo Info(DepthStencilTexture,
			DepthStencilAction,
			nullptr,
			FExclusiveDepthStencil::DepthWrite_StencilWrite);
		Width = DepthStencilTexture->GetSizeX();
		Height = DepthStencilTexture->GetSizeY();
		RHICmdList.BeginRenderPass(Info, Name);
		return true;
	}

	return false;
}

void UCompushadyRasterizer::Clear(const TArray<UCompushadyRTV*>& RTVs, UCompushadyDSV* DSV, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Rasterizer is already running");
		return;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	if (!SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture))
	{
		OnSignaled.ExecuteIfBound(false, "Invalid RTVs");
		return;
	}

	EnqueueToGPU(
		[RenderTargets, RenderTargetsEnabled, DepthStencilTexture](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;
			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::Clear"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Clear_Store, EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil, Width, Height))
			{
				RHICmdList.EndRenderPass();
			}

		}, OnSignaled);
}

void UCompushadyRasterizer::DispatchMesh(const FCompushadyResourceArray& MSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const FIntVector XYZ, const FCompushadyRasterizeConfig& RasterizeConfig, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Rasterizer is already running");
		return;
	}

	if (XYZ.GetMin() <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid Thread Group Size %s"), *XYZ.ToString()));
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(MSResourceArray, MSResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	if (!Compushady::Utils::ValidateResourceBindings(PSResourceArray, PSResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	if (!SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture))
	{
		OnSignaled.ExecuteIfBound(false, "Invalid RTVs");
		return;
	}

	TrackResources(MSResourceArray);
	TrackResources(PSResourceArray);

	EnqueueToGPU(
		[this, XYZ, MSResourceArray, PSResourceArray, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, RasterizeConfig](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;
			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::DispatchMesh"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Load_Store, EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil, Width, Height))
			{
				SetupRasterization_RenderThread(RHICmdList, RasterizeConfig, Width, Height);

				Compushady::Utils::SetupPipelineParameters(RHICmdList, MeshShaderRef, MSResourceArray, MSResourceBindings, true);
				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, {}, true);

				RHICmdList.DispatchMeshShader(XYZ.X, XYZ.Y, XYZ.Z);

				RHICmdList.EndRenderPass();
			}

		}, OnSignaled);
}

bool UCompushadyRasterizer::IsRunning() const
{
	return ICompushadySignalable::IsRunning();
}

void UCompushadyRasterizer::StoreLastSignal(bool bSuccess, const FString& ErrorMessage)
{
	bLastSuccess = bSuccess;
	LastErrorMessages = ErrorMessage;
}