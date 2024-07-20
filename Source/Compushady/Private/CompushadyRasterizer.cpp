// Copyright 2023 - Roberto De Ioris.


#include "CompushadyRasterizer.h"
#include "CommonRenderResources.h"
#include "Compushady.h"
#include "Serialization/ArrayWriter.h"

bool UCompushadyRasterizer::InitVSPSFromHLSL(const TArray<uint8>& VertexShaderCode, const FString& VertexShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	RHIInterfaceType = RHIGetInterfaceType();

	FIntVector ThreadGroupSize;

	TArray<uint8> VertexShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings VertexShaderResourceBindings;
	if (!Compushady::CompileHLSL(VertexShaderCode, VertexShaderEntryPoint, "vs_6_0", VertexShaderByteCode, VertexShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	TArray<uint8> PixelShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings;
	if (!Compushady::CompileHLSL(PixelShaderCode, PixelShaderEntryPoint, "ps_6_0", PixelShaderByteCode, PixelShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	return CreateVSPSRasterizerPipeline(VertexShaderByteCode, PixelShaderByteCode, VertexShaderResourceBindings, PixelShaderResourceBindings, RasterizerConfig, ErrorMessages);
}

bool UCompushadyRasterizer::InitMSPSFromHLSL(const TArray<uint8>& MeshShaderCode, const FString& MeshShaderEntryPoint, const TArray<uint8>& PixelShaderCode, const FString& PixelShaderEntryPoint, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	RHIInterfaceType = RHIGetInterfaceType();

	FIntVector ThreadGroupSize;

	TArray<uint8> MeshShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings MeshShaderResourceBindings;
	if (!Compushady::CompileHLSL(MeshShaderCode, MeshShaderEntryPoint, "ms_6_5", MeshShaderByteCode, MeshShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	TArray<uint8> PixelShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings;
	if (!Compushady::CompileHLSL(PixelShaderCode, PixelShaderEntryPoint, "ps_6_0", PixelShaderByteCode, PixelShaderResourceBindings, ThreadGroupSize, ErrorMessages))
	{
		return false;
	}

	return CreateMSPSRasterizerPipeline(MeshShaderByteCode, PixelShaderByteCode, MeshShaderResourceBindings, PixelShaderResourceBindings, RasterizerConfig, ErrorMessages);
}

bool UCompushadyRasterizer::CreateVSPSRasterizerPipeline(TArray<uint8>& VertexShaderByteCode, TArray<uint8>& PixelShaderByteCode, Compushady::FCompushadyShaderResourceBindings VertexShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	// check for semantics
	if (VertexShaderResourceBindings.InputSemantics.Num() > 0)
	{
		ErrorMessages = FString::Printf(TEXT("Unsupported input semantic in vertex shader: %s/%d"), *(VertexShaderResourceBindings.InputSemantics[0]).Name, VertexShaderResourceBindings.InputSemantics[0].Index);
		return false;
	}

	for (const Compushady::FCompushadyShaderSemantic& Semantic : PixelShaderResourceBindings.InputSemantics)
	{
		if (!VertexShaderResourceBindings.OutputSemantics.Contains(Semantic))
		{
			ErrorMessages = FString::Printf(TEXT("Unknown/Unaligned input semantic in pixel shader: %s/%d (register: %u mask: 0x%x)"), *Semantic.Name, Semantic.Index, Semantic.Register, Semantic.Mask);
			return false;
		}
	}

	if (!Compushady::Utils::CreateResourceBindings(VertexShaderResourceBindings, VSResourceBindings, ErrorMessages))
	{
		return false;
	}

	if (!Compushady::Utils::CreateResourceBindings(PixelShaderResourceBindings, PSResourceBindings, ErrorMessages))
	{
		return false;
	}

	TArray<uint8> VSByteCode;
	FSHAHash VSHash;
	if (!Compushady::ToUnrealShader(VertexShaderByteCode, VSByteCode, VSResourceBindings.NumCBVs, VSResourceBindings.NumSRVs, VSResourceBindings.NumUAVs, VSResourceBindings.NumSamplers, VSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the Vertex Shader";
		return false;
	}

	VertexShaderRef = RHICreateVertexShader(VSByteCode, VSHash);
	if (!VertexShaderRef.IsValid() || !VertexShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Vertex Shader";
		return false;
	}

	VertexShaderRef->SetHash(VSHash);

	TArray<uint8> PSByteCode;
	FSHAHash PSHash;
	if (!Compushady::ToUnrealShader(PixelShaderByteCode, PSByteCode, PSResourceBindings.NumCBVs, PSResourceBindings.NumSRVs, PSResourceBindings.NumUAVs, PSResourceBindings.NumSamplers, PSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the Pixel Shader";
		return false;
	}

	PixelShaderRef = RHICreatePixelShader(PSByteCode, PSHash);
	if (!PixelShaderRef.IsValid() || !PixelShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Pixel Shader";
		return false;
	}

	PixelShaderRef->SetHash(PSHash);

	FillPipelineStateInitializer(RasterizerConfig);

	PipelineStateInitializer.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
	PipelineStateInitializer.BoundShaderState.VertexShaderRHI = VertexShaderRef;
	PipelineStateInitializer.BoundShaderState.PixelShaderRHI = PixelShaderRef;

	return true;
}

void UCompushadyRasterizer::FillPipelineStateInitializer(const FCompushadyRasterizerConfig& RasterizerConfig)
{
	if (RasterizerConfig.FillMode == ECompushadyRasterizerFillMode::Solid)
	{
		switch (RasterizerConfig.CullMode)
		{
		case(ECompushadyRasterizerCullMode::None):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
			break;
		case(ECompushadyRasterizerCullMode::ClockWise):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI();
			break;
		case(ECompushadyRasterizerCullMode::CounterClockWise):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI();
			break;
		}
	}
	else if (RasterizerConfig.FillMode == ECompushadyRasterizerFillMode::Wireframe)
	{
		switch (RasterizerConfig.CullMode)
		{
		case(ECompushadyRasterizerCullMode::None):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_None>::GetRHI();
			break;
		case(ECompushadyRasterizerCullMode::ClockWise):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_CW>::GetRHI();
			break;
		case(ECompushadyRasterizerCullMode::CounterClockWise):
			PipelineStateInitializer.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_CCW>::GetRHI();
			break;
		}
	}

	PipelineStateInitializer.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	PipelineStateInitializer.BlendState = TStaticBlendState<>::GetRHI();
	PipelineStateInitializer.PrimitiveType = PT_TriangleList;
}

bool UCompushadyRasterizer::CreateMSPSRasterizerPipeline(TArray<uint8>& MeshShaderByteCode, TArray<uint8>& PixelShaderByteCode, Compushady::FCompushadyShaderResourceBindings MeshShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings PixelShaderResourceBindings, const FCompushadyRasterizerConfig& RasterizerConfig, FString& ErrorMessages)
{
	// check for semantics
	if (MeshShaderResourceBindings.InputSemantics.Num() > 0)
	{
		ErrorMessages = FString::Printf(TEXT("Unsupported input semantic in mesh shader: %s/%d"), *(MeshShaderResourceBindings.InputSemantics[0]).Name, MeshShaderResourceBindings.InputSemantics[0].Index);
		return false;
	}

	for (const Compushady::FCompushadyShaderSemantic& Semantic : PixelShaderResourceBindings.InputSemantics)
	{
		if (!MeshShaderResourceBindings.OutputSemantics.Contains(Semantic))
		{
			ErrorMessages = FString::Printf(TEXT("Unknown/Unaligned input semantic in pixel shader: %s/%d (register: %u mask: 0x%x)"), *Semantic.Name, Semantic.Index, Semantic.Register, Semantic.Mask);
			return false;
		}
	}

	if (!Compushady::Utils::CreateResourceBindings(MeshShaderResourceBindings, MSResourceBindings, ErrorMessages))
	{
		return false;
	}

	if (!Compushady::Utils::CreateResourceBindings(PixelShaderResourceBindings, PSResourceBindings, ErrorMessages))
	{
		return false;
	}

	TArray<uint8> MSByteCode;
	FSHAHash MSHash;
	if (!Compushady::ToUnrealShader(MeshShaderByteCode, MSByteCode, MSResourceBindings.NumCBVs, MSResourceBindings.NumSRVs, MSResourceBindings.NumUAVs, MSResourceBindings.NumSamplers, MSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the vertex shader";
		return false;
	}

	MeshShaderRef = RHICreateMeshShader(MSByteCode, MSHash);
	if (!MeshShaderRef.IsValid() || !MeshShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Mesh Shader";
		return false;
	}

	MeshShaderRef->SetHash(MSHash);

	TArray<uint8> PSByteCode;
	FSHAHash PSHash;
	if (!Compushady::ToUnrealShader(PixelShaderByteCode, PSByteCode, PSResourceBindings.NumCBVs, PSResourceBindings.NumSRVs, PSResourceBindings.NumUAVs, PSResourceBindings.NumSamplers, PSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the pixel shader";
		return false;
	}

	PixelShaderRef = RHICreatePixelShader(PSByteCode, PSHash);
	if (!PixelShaderRef.IsValid() || !PixelShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create Pixel Shader";
		return false;
	}

	PixelShaderRef->SetHash(PSHash);

	FillPipelineStateInitializer(RasterizerConfig);

	PipelineStateInitializer.BoundShaderState.VertexDeclarationRHI = nullptr;
	PipelineStateInitializer.BoundShaderState.SetMeshShader(MeshShaderRef);
	PipelineStateInitializer.BoundShaderState.PixelShaderRHI = PixelShaderRef;

	return true;
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

	if (!CheckResourceBindings(VSResourceArray, VSResourceBindings, OnSignaled))
	{
		return;
	}

	if (!CheckResourceBindings(PSResourceArray, PSResourceBindings, OnSignaled))
	{
		return;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture);

	TrackResources(VSResourceArray);
	TrackResources(PSResourceArray);

	EnqueueToGPU(
		[this, NumVertices, NumInstances, VSResourceArray, PSResourceArray, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, RasterizeConfig](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;

			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::Draw"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Load_Store, EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil, Width, Height))
			{
				SetupRasterization(RHICmdList, RasterizeConfig, Width, Height);

				Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings);
				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, {});

				RHICmdList.DrawPrimitive(0, NumVertices / 3, NumInstances);

				RHICmdList.EndRenderPass();
			}
		}, OnSignaled);
}

void UCompushadyRasterizer::SetupRasterization(FRHICommandListImmediate& RHICmdList, const FCompushadyRasterizeConfig& RasterizeConfig, const int32 Width, const int32 Height)
{
	float ViewportMinX = RasterizeConfig.Viewport.Min.X > 0 ? RasterizeConfig.Viewport.Min.X : 0;
	float ViewportMinY = RasterizeConfig.Viewport.Min.Y > 0 ? RasterizeConfig.Viewport.Min.Y : 0;
	float ViewportMinZ = RasterizeConfig.Viewport.Min.Z > 0 ? RasterizeConfig.Viewport.Min.Z : 0;
	float ViewportMaxX = RasterizeConfig.Viewport.Max.X > 0 ? RasterizeConfig.Viewport.Max.X : Width;
	float ViewportMaxY = RasterizeConfig.Viewport.Max.Y > 0 ? RasterizeConfig.Viewport.Max.Y : Height;
	float ViewportMaxZ = RasterizeConfig.Viewport.Max.Z > 0 ? RasterizeConfig.Viewport.Max.Z : 1.0;

	RHICmdList.SetViewport(ViewportMinX, ViewportMinY, ViewportMinZ, ViewportMaxX, ViewportMaxY, ViewportMaxZ);
	RHICmdList.SetScissorRect(RasterizeConfig.Scissor.GetArea() > 0, RasterizeConfig.Scissor.Min.X, RasterizeConfig.Scissor.Min.Y, RasterizeConfig.Scissor.Max.X, RasterizeConfig.Scissor.Max.Y);
	RHICmdList.SetStencilRef(RasterizeConfig.StencilValue);

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

	if (!CheckResourceBindings(VSResourceArray, VSResourceBindings, OnSignaled))
	{
		return;
	}

	if (!CheckResourceBindings(PSResourceArray, PSResourceBindings, OnSignaled))
	{
		return;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture);

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
				SetupRasterization(RHICmdList, RasterizeConfig, Width, Height);

				Compushady::Utils::SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings);
				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, {});

				RHICmdList.DrawPrimitiveIndirect(RHICommandBuffer, Offset);

				RHICmdList.EndRenderPass();
			}

		}, OnSignaled);
}

void UCompushadyRasterizer::SetupRenderTargets(const TArray<UCompushadyRTV*>& RTVs, UCompushadyDSV* DSV, TStaticArray<FRHITexture*, 8>& RenderTargets, int32& RenderTargetsEnabled, FRHITexture*& DepthStencilTexture)
{
	for (int32 Index = 0; Index < RTVs.Num(); Index++)
	{
		RenderTargets[Index] = RTVs[Index]->GetTextureRHI();
		RenderTargetsEnabled++;
		TrackResource(RTVs[Index]);
	}

	if (DSV)
	{
		DepthStencilTexture = DSV->GetTextureRHI();
		TrackResource(DSV);
	}
}

bool UCompushadyRasterizer::BeginRenderPass_RenderThread(const TCHAR* Name, FRHICommandListImmediate& RHICmdList, const TStaticArray<FRHITexture*, 8>& RenderTargets, const int32 RenderTargetsEnabled, FRHITexture* DepthStencilTexture, const ERenderTargetActions ColorAction, const EDepthStencilTargetActions DepthStencilAction, uint32& Width, uint32& Height)
{
	for (int32 RenderTargetIndex = 0; RenderTargetIndex < RenderTargetsEnabled; RenderTargetIndex++)
	{
		RHICmdList.Transition(FRHITransitionInfo(RenderTargets[RenderTargetIndex], ERHIAccess::Unknown, ERHIAccess::RTV));
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
	SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture);

	EnqueueToGPU(
		[RenderTargets, RenderTargetsEnabled, DepthStencilTexture](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;
			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::Clear"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Clear_DontStore, EDepthStencilTargetActions::ClearDepthStencil_DontStoreDepthStencil, Width, Height))
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

	if (!CheckResourceBindings(MSResourceArray, MSResourceBindings, OnSignaled))
	{
		return;
	}

	if (!CheckResourceBindings(PSResourceArray, PSResourceBindings, OnSignaled))
	{
		return;
	}

	TStaticArray<FRHITexture*, 8> RenderTargets = {};
	int32 RenderTargetsEnabled = 0;
	FRHITexture* DepthStencilTexture = nullptr;
	SetupRenderTargets(RTVs, DSV, RenderTargets, RenderTargetsEnabled, DepthStencilTexture);

	TrackResources(MSResourceArray);
	TrackResources(PSResourceArray);

	EnqueueToGPU(
		[this, XYZ, MSResourceArray, PSResourceArray, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, RasterizeConfig](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Width = 0;
			uint32 Height = 0;
			if (BeginRenderPass_RenderThread(TEXT("UCompushadyRasterizer::DispatchMesh"), RHICmdList, RenderTargets, RenderTargetsEnabled, DepthStencilTexture, ERenderTargetActions::Load_Store, EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil, Width, Height))
			{
				SetupRasterization(RHICmdList, RasterizeConfig, Width, Height);

				Compushady::Utils::SetupPipelineParameters(RHICmdList, MeshShaderRef, MSResourceArray, MSResourceBindings);
				Compushady::Utils::SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings, {});

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