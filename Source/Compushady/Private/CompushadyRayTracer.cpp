// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadyRayTracer.h"
#include "CommonRenderResources.h"
#include "Compushady.h"
#include "Serialization/ArrayWriter.h"
#include "FXRenderingUtils.h"

bool UCompushadyRayTracer::InitFromHLSL(const TArray<uint8>& RayGenShaderCode, const FString& RayGenShaderEntryPoint, const TArray<uint8>& RayMissShaderCode, const FString& RayMissShaderEntryPoint, const TArray<uint8>& RayHitGroupShaderCode, const FString& RayHitGroupShaderEntryPoint, FString& ErrorMessages)
{
	RHIInterfaceType = RHIGetInterfaceType();

	TArray<uint8> RayGenShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings RayGenShaderResourceBindings;
	if (!Compushady::CompileHLSL(RayGenShaderCode, RayGenShaderEntryPoint, "lib_6_3", RayGenShaderByteCode, ErrorMessages, false))
	{
		return false;
	}

	TArray<uint8> RayMissShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings RayMissShaderResourceBindings;
	if (!Compushady::CompileHLSL(RayMissShaderCode, RayMissShaderEntryPoint, "lib_6_3", RayMissShaderByteCode, ErrorMessages, false))
	{
		return false;
	}

	TArray<uint8> RayHitGroupShaderByteCode;
	Compushady::FCompushadyShaderResourceBindings RayHitGroupShaderResourceBindings;
	if (!Compushady::CompileHLSL(RayHitGroupShaderCode, RayHitGroupShaderEntryPoint, "lib_6_3", RayHitGroupShaderByteCode, ErrorMessages, false))
	{
		return false;
	}

	return CreateRayTracerPipeline(RayGenShaderByteCode, RayMissShaderByteCode, RayHitGroupShaderByteCode, RayGenShaderResourceBindings, RayMissShaderResourceBindings, RayHitGroupShaderResourceBindings, ErrorMessages);
}

bool UCompushadyRayTracer::CreateRayTracerPipeline(TArray<uint8>& RayGenShaderByteCode, TArray<uint8>& RayMissShaderByteCode, TArray<uint8>& RayHitGroupShaderByteCode, Compushady::FCompushadyShaderResourceBindings RGShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings RMShaderResourceBindings, Compushady::FCompushadyShaderResourceBindings RHGShaderResourceBindings, FString& ErrorMessages)
{
	if (!Compushady::Utils::CreateResourceBindings(RGShaderResourceBindings, RayGenResourceBindings, ErrorMessages))
	{
		return false;
	}

	if (!Compushady::Utils::CreateResourceBindings(RMShaderResourceBindings, RayMissResourceBindings, ErrorMessages))
	{
		return false;
	}

	if (!Compushady::Utils::CreateResourceBindings(RHGShaderResourceBindings, RayHitGroupResourceBindings, ErrorMessages))
	{
		return false;
	}

	/** RayGen Shader */

	TArray<uint8> RGSByteCode;
	FSHAHash RGSHash;
	if (!Compushady::ToUnrealShader(RayGenShaderByteCode, RGSByteCode, RayGenResourceBindings.NumCBVs, RayGenResourceBindings.NumSRVs, RayGenResourceBindings.NumUAVs, RayGenResourceBindings.NumSamplers, RGSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the RayGen Shader";
		return false;
	}

	RayGenShaderRef = RHICreateRayTracingShader(RGSByteCode, RGSHash, EShaderFrequency::SF_RayGen);
	if (!RayGenShaderRef.IsValid() || !RayGenShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create RayGen Shader";
		return false;
	}

	RayGenShaderRef->SetHash(RGSHash);

	/** RayMiss Shader */

	TArray<uint8> RMSByteCode;
	FSHAHash RMSHash;
	if (!Compushady::ToUnrealShader(RayMissShaderByteCode, RMSByteCode, RayMissResourceBindings.NumCBVs, RayMissResourceBindings.NumSRVs, RayMissResourceBindings.NumUAVs, RayMissResourceBindings.NumSamplers, RMSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the RayMiss Shader";
		return false;
	}

	RayMissShaderRef = RHICreateRayTracingShader(RMSByteCode, RMSHash, EShaderFrequency::SF_RayMiss);
	if (!RayMissShaderRef.IsValid() || !RayMissShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create RayMiss Shader";
		return false;
	}

	RayMissShaderRef->SetHash(RMSHash);

	/** RayHitGroup Shader */

	TArray<uint8> RHGSByteCode;
	FSHAHash RHGSHash;
	if (!Compushady::ToUnrealShader(RayHitGroupShaderByteCode, RHGSByteCode, RayHitGroupResourceBindings.NumCBVs, RayHitGroupResourceBindings.NumSRVs, RayHitGroupResourceBindings.NumUAVs, RayHitGroupResourceBindings.NumSamplers, RHGSHash))
	{
		ErrorMessages = "Unable to add Unreal metadata to the RayHitGroup Shader";
		return false;
	}

	RayHitGroupShaderRef = RHICreateRayTracingShader(RHGSByteCode, RHGSHash, EShaderFrequency::SF_RayHitGroup);
	if (!RayHitGroupShaderRef.IsValid() || !RayHitGroupShaderRef->IsValid())
	{
		ErrorMessages = "Unable to create RayHitGroup Shader";
		return false;
	}

	RayHitGroupShaderRef->SetHash(RHGSHash);

	FRHIRayTracingShader* RayGenShaderTable[] = { RayGenShaderRef };
	PipelineStateInitializer.SetRayGenShaderTable(RayGenShaderTable);

	FRHIRayTracingShader* RayMissShaderTable[] = { RayMissShaderRef };
	PipelineStateInitializer.SetMissShaderTable(RayMissShaderTable);

	FRHIRayTracingShader* RayHitGroupShaderTable[] = { RayHitGroupShaderRef };
	PipelineStateInitializer.SetHitGroupTable(RayHitGroupShaderTable);

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateRayTracerPipelineState)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			PipelineState = PipelineStateCache::GetAndOrCreateRayTracingPipelineState(RHICmdList, PipelineStateInitializer);
		});

	FlushRenderingCommands();

	if (!PipelineState)
	{
		ErrorMessages = "Unable to create RayTracer Pipeline State";
		return false;
	}

	return true;
}

void UCompushadyRayTracer::DispatchRays(const FCompushadyResourceArray& ResourceArray, const FIntVector XYZ, const FCompushadySignaled& OnSignaled)
{
#if COMPUSHADY_UE_VERSION >= 53
	if (IsRunning())
	{
		OnSignaled.ExecuteIfBound(false, "The RayTracer is already running");
		return;
	}

	if (XYZ.GetMin() <= 0)
	{
		OnSignaled.ExecuteIfBound(false, FString::Printf(TEXT("Invalid Thread Group Size %s"), *XYZ.ToString()));
		return;
	}

	FString ErrorMessages;
	if (!Compushady::Utils::ValidateResourceBindings(ResourceArray, RayGenResourceBindings, ErrorMessages))
	{
		OnSignaled.ExecuteIfBound(false, ErrorMessages);
		return;
	}

	TrackResources(ResourceArray);

	EnqueueToGPU(
		[this, XYZ, ResourceArray](FRHICommandListImmediate& RHICmdList)
		{
			FRayTracingShaderBindings Bindings;
			RHICmdList.RayTraceDispatch(PipelineState, RayGenShaderRef, UE::FXRenderingUtils::RayTracing::GetRayTracingScene(GetWorld()->Scene), Bindings, XYZ.X, XYZ.Y);
		}, OnSignaled);
#endif
}

bool UCompushadyRayTracer::IsRunning() const
{
	return ICompushadySignalable::IsRunning();
}

void UCompushadyRayTracer::StoreLastSignal(bool bSuccess, const FString& ErrorMessage)
{
	bLastSuccess = bSuccess;
	LastErrorMessages = ErrorMessage;

}