// Copyright 2023-2025 - Roberto De Ioris.


#include "CompushadyCompute.h"
#include "Compushady.h"
#include "FXSystem.h"
#include "EngineModule.h"
#include "Serialization/ArrayWriter.h"

bool UCompushadyCompute::InitFromHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	FRenderQueryRHIRef Query = RHICreateRenderQuery(ERenderQueryType::RQT_AbsoluteTime);
	ComputeShaderRef = Compushady::Utils::CreateComputeShaderFromHLSL(ShaderCode, EntryPoint, ResourceBindings, ThreadGroupSize, ErrorMessages);
	return ComputeShaderRef != nullptr;
}

bool UCompushadyCompute::InitFromGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, FString& ErrorMessages)
{
	ComputeShaderRef = Compushady::Utils::CreateComputeShaderFromGLSL(ShaderCode, EntryPoint, ResourceBindings, ThreadGroupSize, ErrorMessages);
	return ComputeShaderRef != nullptr;
}

bool UCompushadyCompute::InitFromSPIRV(const TArray<uint8>& ShaderCode, FString& ErrorMessages)
{
	ComputeShaderRef = Compushady::Utils::CreateComputeShaderFromSPIRVBlob(ShaderCode, ResourceBindings, ThreadGroupSize, ErrorMessages);
	return ComputeShaderRef != nullptr;
}

bool UCompushadyCompute::InitFromDXIL(const TArray<uint8>& ShaderCode, FString& ErrorMessages)
{
	if (RHIGetInterfaceType() != ERHIInterfaceType::D3D12)
	{
		ErrorMessages = "DXIL shaders are currently supported only on Direct3D12";
		return false;
	}

	TArray<uint8> ByteCode = ShaderCode;
	Compushady::FCompushadyShaderResourceBindings ShaderResourceBindings;

	if (!Compushady::FixupDXIL(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages, false))
	{
		return false;
	}

	return false;
}

void UCompushadyCompute::Dispatch_RenderThread(FRHICommandList& RHICmdList, const FCompushadyResourceArray& ResourceArray, const FIntVector& XYZ)
{
	SetComputePipelineState(RHICmdList, ComputeShaderRef);
	Compushady::Utils::SetupPipelineParameters(RHICmdList, ComputeShaderRef, ResourceArray, ResourceBindings, true);

	RHICmdList.DispatchComputeShader(XYZ.X, XYZ.Y, XYZ.Z);
}

void UCompushadyCompute::DispatchIndirect_RenderThread(FRHICommandList& RHICmdList, const FCompushadyResourceArray& ResourceArray, FBufferRHIRef BufferRHIRef, const int32 Offset)
{
	SetComputePipelineState(RHICmdList, ComputeShaderRef);
	Compushady::Utils::SetupPipelineParameters(RHICmdList, ComputeShaderRef, ResourceArray, ResourceBindings, true);

	RHICmdList.Transition(FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::IndirectArgs));
	RHICmdList.DispatchIndirectComputeShader(BufferRHIRef, Offset);
}

void UCompushadyCompute::Dispatch(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Compute is already running");
		return;
	}

	if (XYZ.X <= 0 || XYZ.Y <= 0 || XYZ.Z <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid ThreadGroupCount %s"), *XYZ.ToString()));
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(ResourceArray, ResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	TrackResources(ResourceArray);

	EnqueueToGPU(
		[this, ResourceArray, XYZ](FRHICommandListImmediate& RHICmdList)
		{
			Dispatch_RenderThread(RHICmdList, ResourceArray, XYZ);
		}, OnSignaled);
}

void UCompushadyCompute::DispatchPostOpaqueRender_Delegate(FPostOpaqueRenderParameters& Parameters)
{
	Parameters.GraphBuilder->AddPass(RDG_EVENT_NAME("UCompushadyCompute::DispatchPostOpaqueRender"), ERDGPassFlags::None, [this](FRHICommandList& RHICmdList)
		{
			Dispatch_RenderThread(RHICmdList, PostOpaqueRenderResourceArray, PostOpaqueRenderXYZ);
		});

	Parameters.GraphBuilder->FlushSetupQueue();
	GetRendererModule().RemovePostOpaqueRenderDelegate(PostOpaqueRenderDelegateHandle);
	BeginFence(PostOpaqueRenderOnSignaled);
}

void UCompushadyCompute::DispatchPostOpaqueRender(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
{
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The Compute is already running");
		return;
	}

	if (XYZ.X <= 0 || XYZ.Y <= 0 || XYZ.Z <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid ThreadGroupCount %s"), *XYZ.ToString()));
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(ResourceArray, ResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	TrackResources(ResourceArray);

	PostOpaqueRenderResourceArray = ResourceArray;
	PostOpaqueRenderXYZ = XYZ;
	PostOpaqueRenderOnSignaled = OnSignaled;

	PostOpaqueRenderDelegateHandle = GetRendererModule().RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateUObject(this, &UCompushadyCompute::DispatchPostOpaqueRender_Delegate));
}

void UCompushadyCompute::DispatchAndProfile(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaledAndProfiled& OnSignaledAndProfiled)
{
	if (IsRunning())
	{
		OnSignaledAndProfiled.ExecuteIfBound(false, 0, "The Compute is already running");
		return;
	}

	if (XYZ.X <= 0 || XYZ.Y <= 0 || XYZ.Z <= 0)
	{
		OnSignaledAndProfiled.ExecuteIfBound(false, 0, FString::Printf(TEXT("Invalid ThreadGroupCount %s"), *XYZ.ToString()));
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(ResourceArray, ResourceBindings, ErrorMessages))
	{
		OnSignaledAndProfiled.ExecuteIfBound(false, 0, ErrorMessages);
		return;
	}

	TrackResources(ResourceArray);

	EnqueueToGPUAndProfile(
		[this, ResourceArray, XYZ](FRHICommandListImmediate& RHICmdList)
		{
			Dispatch_RenderThread(RHICmdList, ResourceArray, XYZ);
		}, OnSignaledAndProfiled);
}

void UCompushadyCompute::DispatchByMapAndProfile(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FIntVector XYZ, const FCompushadySignaledAndProfiled& OnSignaledAndProfiled)
{
	FCompushadyResourceArray ResourceArray;
	FString ErrorMessages;

	if (!Compushady::Utils::ValidateResourceBindingsMap(ResourceMap, ResourceBindings, ResourceArray, ErrorMessages))
	{
		OnSignaledAndProfiled.ExecuteIfBound(false, 0, ErrorMessages);
		return;
	}

	DispatchAndProfile(ResourceArray, XYZ, OnSignaledAndProfiled);
}

bool UCompushadyCompute::DispatchSync(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, FString& ErrorMessages)
{
	if (IsRunning())
	{
		ErrorMessages = "The Compute is already running";
		return false;
	}

	if (XYZ.X <= 0 || XYZ.Y <= 0 || XYZ.Z <= 0)
	{
		ErrorMessages = FString::Printf(TEXT("Invalid ThreadGroupCount %s"), *XYZ.ToString());
		return false;
	}

	if (!Compushady::Utils::ValidateResourceBindings(ResourceArray, ResourceBindings, ErrorMessages))
	{
		return false;
	}

	EnqueueToGPUSync(
		[this, ResourceArray, XYZ](FRHICommandListImmediate& RHICmdList)
		{
			Dispatch_RenderThread(RHICmdList, ResourceArray, XYZ);
		});

	return true;
}

void UCompushadyCompute::DispatchByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
{
	FCompushadyResourceArray ResourceArray;
	FString ErrorMessages;

	if (!Compushady::Utils::ValidateResourceBindingsMap(ResourceMap, ResourceBindings, ResourceArray, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	Dispatch(ResourceArray, XYZ, OnSignaled);
}

void UCompushadyCompute::DispatchByMapPostOpaqueRender(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
{
	FCompushadyResourceArray ResourceArray;
	FString ErrorMessages;

	if (!Compushady::Utils::ValidateResourceBindingsMap(ResourceMap, ResourceBindings, ResourceArray, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	DispatchPostOpaqueRender(ResourceArray, XYZ, OnSignaled);
}

bool UCompushadyCompute::DispatchByMapSync(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, const FIntVector XYZ, FString& ErrorMessages)
{
	FCompushadyResourceArray ResourceArray;

	if (!Compushady::Utils::ValidateResourceBindingsMap(ResourceMap, ResourceBindings, ResourceArray, ErrorMessages))
	{
		return false;
	}

	return DispatchSync(ResourceArray, XYZ, ErrorMessages);
}

void UCompushadyCompute::DispatchIndirect(const FCompushadyResourceArray& ResourceArray, UCompushadyResource* CommandBuffer, const int32 Offset, const FCompushadySignaled& OnSignaled)
{
	if (!CommandBuffer)
	{
		OnSignaled.ExecuteIfBound(false, "CommandBuffer is NULL");
		return;
	}

	FBufferRHIRef BufferRHIRef = CommandBuffer->GetBufferRHI();
	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
	{
		OnSignaled.ExecuteIfBound(false, "Invalid Indirect Buffer");
		return;
	}

	if (BufferRHIRef->GetSize() < (sizeof(uint32) * 3))
	{
		OnSignaled.ExecuteIfBound(false, "Invalid Indirect Buffer size (expected sizeof(uint32) * 3)");
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(ResourceArray, ResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	TrackResources(ResourceArray);

	EnqueueToGPU(
		[this, ResourceArray, BufferRHIRef, Offset](FRHICommandListImmediate& RHICmdList)
		{
			DispatchIndirect_RenderThread(RHICmdList, ResourceArray, BufferRHIRef, Offset);
		}, OnSignaled);
}

bool UCompushadyCompute::DispatchIndirectSync(const FCompushadyResourceArray& ResourceArray, UCompushadyResource* CommandBuffer, const int32 Offset, FString& ErrorMessages)
{
	if (!CommandBuffer)
	{
		ErrorMessages = "CommandBuffer is NULL";
		return false;
	}

	FBufferRHIRef BufferRHIRef = CommandBuffer->GetBufferRHI();
	if (!BufferRHIRef.IsValid() || !BufferRHIRef->IsValid())
	{
		ErrorMessages = "Invalid Indirect Buffer";
		return false;
	}

	if (BufferRHIRef->GetSize() < (sizeof(uint32) * 3))
	{
		ErrorMessages = "Invalid Indirect Buffer size (expected sizeof(uint32) * 3)";
		return false;
	}

	if (!Compushady::Utils::ValidateResourceBindings(ResourceArray, ResourceBindings, ErrorMessages))
	{
		return false;
	}

	TrackResources(ResourceArray);

	EnqueueToGPUSync(
		[this, ResourceArray, BufferRHIRef, Offset](FRHICommandListImmediate& RHICmdList)
		{
			DispatchIndirect_RenderThread(RHICmdList, ResourceArray, BufferRHIRef, Offset);
		});

	return true;
}

void UCompushadyCompute::DispatchIndirectByMap(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, UCompushadyResource* CommandBuffer, const int32 Offset, const FCompushadySignaled& OnSignaled)
{
	FCompushadyResourceArray ResourceArray;
	FString ErrorMessages;

	if (!Compushady::Utils::ValidateResourceBindingsMap(ResourceMap, ResourceBindings, ResourceArray, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	DispatchIndirect(ResourceArray, CommandBuffer, Offset, OnSignaled);
}

bool UCompushadyCompute::DispatchIndirectByMapSync(const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, UCompushadyResource* CommandBuffer, const int32 Offset, FString& ErrorMessages)
{
	FCompushadyResourceArray ResourceArray;

	if (!Compushady::Utils::ValidateResourceBindingsMap(ResourceMap, ResourceBindings, ResourceArray, ErrorMessages))
	{
		return false;
	}

	return DispatchIndirectSync(ResourceArray, CommandBuffer, Offset, ErrorMessages);
}

bool UCompushadyCompute::IsRunning() const
{
	return ICompushadySignalable::IsRunning();
}

FIntVector UCompushadyCompute::GetThreadGroupSize() const
{
	return ThreadGroupSize;
}

void UCompushadyCompute::StoreLastSignal(bool bSuccess, const FString& ErrorMessage)
{
	bLastSuccess = bSuccess;
	LastErrorMessages = ErrorMessage;
}