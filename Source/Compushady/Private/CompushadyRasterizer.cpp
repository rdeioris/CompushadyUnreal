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

	if (!CreateResourceBindings(VertexShaderResourceBindings, VSResourceBindings, ErrorMessages))
	{
		return false;
	}

	if (!CreateResourceBindings(PixelShaderResourceBindings, PSResourceBindings, ErrorMessages))
	{
		return false;
	}

	TArray<uint8> VSByteCode;
	FSHAHash VSHash;
	if (!Compushady::ToUnrealShader(VertexShaderByteCode, VSByteCode, VSResourceBindings.NumCBVs, VSResourceBindings.NumSRVs, VSResourceBindings.NumUAVs, VSHash))
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
	if (!Compushady::ToUnrealShader(PixelShaderByteCode, PSByteCode, PSResourceBindings.NumCBVs, PSResourceBindings.NumSRVs, PSResourceBindings.NumUAVs, PSHash))
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

	InitFence(this);

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

	if (!CreateResourceBindings(MeshShaderResourceBindings, MSResourceBindings, ErrorMessages))
	{
		return false;
	}

	if (!CreateResourceBindings(PixelShaderResourceBindings, PSResourceBindings, ErrorMessages))
	{
		return false;
	}

	TArray<uint8> MSByteCode;
	FSHAHash MSHash;
	if (!Compushady::ToUnrealShader(MeshShaderByteCode, MSByteCode, MSResourceBindings.NumCBVs, MSResourceBindings.NumSRVs, MSResourceBindings.NumUAVs, MSHash))
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
	if (!Compushady::ToUnrealShader(PixelShaderByteCode, PSByteCode, PSResourceBindings.NumCBVs, PSResourceBindings.NumSRVs, PSResourceBindings.NumUAVs, PSHash))
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

	InitFence(this);

	return true;
}

void UCompushadyRasterizer::Draw(const FCompushadyResourceArray& VSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, UCompushadyDSV* DSV, const int32 NumVertices, const int32 NumInstances, const bool bClearColor, const bool bClearDepthStencil, const FCompushadySignaled& OnSignaled)
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

	if (RTVs.Num() < 1 || RTVs.Num() > 8)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid number of RTVs %d"), RTVs.Num()));
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
	uint32 RenderTargetsEnabled = 0;
	//PipelineStateInitializer.RenderTargetsEnabled = 0;
	for (int32 Index = 0; Index < RTVs.Num(); Index++)
	{
		RenderTargets[Index] = RTVs[Index]->GetTextureRHI();
		RenderTargetsEnabled++;
		//PipelineStateInitializer.RenderTargetsEnabled++;
		//PipelineStateInitializer.RenderTargetFormats[Index] = RTVs[Index]->GetTexturePixelFormat();
		TrackResource(RTVs[Index]);
	}

	TrackResources(VSResourceArray);
	TrackResources(PSResourceArray);

	EnqueueToGPU(
		[this, NumVertices, NumInstances, VSResourceArray, PSResourceArray, RenderTargets, RenderTargetsEnabled, bClearColor, bClearDepthStencil](FRHICommandListImmediate& RHICmdList)
		{
			for (uint32 RenderTargetIndex = 0; RenderTargetIndex < RenderTargetsEnabled; RenderTargetIndex++)
			{
				RHICmdList.Transition(FRHITransitionInfo(RenderTargets[RenderTargetIndex], ERHIAccess::Unknown, ERHIAccess::RTV));
				if (bClearColor)
				{
					ClearRenderTarget(RHICmdList, RenderTargets[RenderTargetIndex]);
				}
			}

			FRHIRenderPassInfo PassInfo(RenderTargetsEnabled, const_cast<FRHITexture**>(RenderTargets.GetData()), ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(PassInfo, TEXT("UCompushadyRasterizer::Draw"));

			//RHICmdList.SetViewport(0, 0, 0.0f, RenderTargets[0]->GetDesc().Extent.X, RenderTargets[0]->GetDesc().Extent.Y, 1.0f);
			RHICmdList.SetViewport(0, 0, 0.0f, 1024, 1024, 1.0f);
			RHICmdList.SetScissorRect(false, 0, 0, 0, 0);
			RHICmdList.SetStencilRef(0);

			RHICmdList.ApplyCachedRenderTargets(PipelineStateInitializer);

			SetGraphicsPipelineState(RHICmdList, PipelineStateInitializer, 0);// , EApplyRendertargetOption::DoNothing, false, EPSOPrecacheResult::Untracked);

			SetupPipelineParameters(RHICmdList, VertexShaderRef, VSResourceArray, VSResourceBindings);
			SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings);

			RHICmdList.DrawPrimitive(0, NumVertices / 3, NumInstances);

			RHICmdList.EndRenderPass();

		}, OnSignaled);
}

void UCompushadyRasterizer::DispatchMesh(const FCompushadyResourceArray& MSResourceArray, const FCompushadyResourceArray& PSResourceArray, const TArray<UCompushadyRTV*> RTVs, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
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

	if (RTVs.Num() < 1 || RTVs.Num() > 8)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid number of RTVs %d"), RTVs.Num()));
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
	PipelineStateInitializer.RenderTargetsEnabled = 0;
	for (int32 Index = 0; Index < RTVs.Num(); Index++)
	{
		RenderTargets[Index] = RTVs[Index]->GetTextureRHI();
		PipelineStateInitializer.RenderTargetsEnabled++;
		PipelineStateInitializer.RenderTargetFormats[Index] = RTVs[Index]->GetTexturePixelFormat();
		TrackResource(RTVs[Index]);
	}

	TrackResources(MSResourceArray);
	TrackResources(PSResourceArray);

	EnqueueToGPU(
		[this, XYZ, MSResourceArray, PSResourceArray, RenderTargets](FRHICommandListImmediate& RHICmdList)
		{
			FRHIRenderPassInfo PassInfo(PipelineStateInitializer.RenderTargetsEnabled, const_cast<FRHITexture**>(RenderTargets.GetData()), ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(PassInfo, TEXT("UCompushadyRasterizer::DispatchMesh"));
			RHICmdList.SetViewport(0, 0, 0.0f, RenderTargets[0]->GetDesc().Extent.X, RenderTargets[0]->GetDesc().Extent.Y, 1.0f);

			SetGraphicsPipelineState(RHICmdList, PipelineStateInitializer, 0);
			SetupPipelineParameters(RHICmdList, MeshShaderRef, MSResourceArray, MSResourceBindings);
			SetupPipelineParameters(RHICmdList, PixelShaderRef, PSResourceArray, PSResourceBindings);

			RHICmdList.DispatchMeshShader(XYZ.X, XYZ.Y, XYZ.Z);

			RHICmdList.EndRenderPass();
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