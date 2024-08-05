// Copyright 2023-2024 - Roberto De Ioris.

#include "CompushadySRV.h"
#include "FXRenderingUtils.h"

bool UCompushadySRV::InitializeFromTexture(FTextureRHIRef InTextureRHIRef)
{
	if (!InTextureRHIRef)
	{
		return false;
	}

	TextureRHIRef = InTextureRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateShaderResourceView)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			SRVRHIRef = COMPUSHADY_CREATE_SRV(TextureRHIRef, 0);
		});

	FlushRenderingCommands();

	if (!SRVRHIRef)
	{
		return false;
	}

	if (InTextureRHIRef->GetOwnerName() == NAME_None)
	{
		InTextureRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::SRVMask);

	return true;
}

bool UCompushadySRV::InitializeFromTextureAdvanced(FTextureRHIRef InTextureRHIRef, const int32 Slice, const int32 SlicesNum, const int32 MipLevel, const int32 MipsNum, const EPixelFormat PixelFormat)
{
	if (!InTextureRHIRef)
	{
		return false;
	}

	if (PixelFormat != EPixelFormat::PF_Unknown && !GPixelFormats[PixelFormat].Supported)
	{
		return false;
	}

	if (Slice < 0 || Slice >= InTextureRHIRef->GetDesc().ArraySize || MipLevel < 0 || MipLevel >= InTextureRHIRef->GetDesc().NumMips)
	{
		return false;
	}

	if (Slice + SlicesNum > InTextureRHIRef->GetDesc().ArraySize || MipLevel + MipsNum > InTextureRHIRef->GetDesc().NumMips)
	{
		return false;
	}

	if (PixelFormat != EPixelFormat::PF_Unknown)
	{
		if (GPixelFormats[InTextureRHIRef->GetDesc().Format].BlockBytes != GPixelFormats[PixelFormat].BlockBytes)
		{
			return false;
		}
	}

	TextureRHIRef = InTextureRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateShaderResourceView)(
		[this, Slice, SlicesNum, MipLevel, MipsNum, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			FRHITextureSRVCreateInfo SRVCreateInfo;
			SRVCreateInfo.DimensionOverride = TextureRHIRef->GetDesc().Dimension;
			SRVCreateInfo.Format = PixelFormat;
			SRVCreateInfo.FirstArraySlice = Slice;
			SRVCreateInfo.NumArraySlices = SlicesNum;
			SRVCreateInfo.MipLevel = MipLevel;
			SRVCreateInfo.NumMipLevels = MipsNum;
			SRVRHIRef = COMPUSHADY_CREATE_SRV(TextureRHIRef, SRVCreateInfo);
		});

	FlushRenderingCommands();

	if (!SRVRHIRef)
	{
		return false;
	}

	if (InTextureRHIRef->GetOwnerName() == NAME_None)
	{
		InTextureRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(TextureRHIRef, ERHIAccess::Unknown, ERHIAccess::SRVMask);

	return true;
}

bool UCompushadySRV::InitializeFromSceneTexture(const ECompushadySceneTexture InSceneTexture)
{
	if (InSceneTexture == ECompushadySceneTexture::None)
	{
		return false;
	}

	SceneTexture = InSceneTexture;

	return true;
}

bool UCompushadySRV::InitializeFromWorldSceneAccelerationStructure(UWorld* World)
{
#if COMPUSHADY_UE_VERSION >= 53
	FSceneInterface* Scene = World->Scene;

	if (!UE::FXRenderingUtils::RayTracing::HasRayTracingScene(Scene))
	{
		return false;
	}

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateShaderResourceView)(
		[this, Scene](FRHICommandListImmediate& RHICmdList)
		{
			SRVRHIRef = UE::FXRenderingUtils::RayTracing::GetRayTracingSceneView(RHICmdList, Scene);
		});

	FlushRenderingCommands();

	if (!SRVRHIRef)
	{
		return false;
	}

	RHITransitionInfo = FRHITransitionInfo(UE::FXRenderingUtils::RayTracing::GetRayTracingScene(Scene), ERHIAccess::Unknown, ERHIAccess::BVHRead);

	return true;
#else
	return false;
#endif
}

bool UCompushadySRV::InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const EPixelFormat PixelFormat)
{
	if (!InBufferRHIRef)
	{
		return false;
	}

	BufferRHIRef = InBufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateShaderResourceView)(
		[this, PixelFormat](FRHICommandListImmediate& RHICmdList)
		{
			SRVRHIRef = COMPUSHADY_CREATE_SRV(BufferRHIRef, GPixelFormats[PixelFormat].BlockBytes, PixelFormat);
		});

	FlushRenderingCommands();

	if (!SRVRHIRef)
	{
		return false;
	}

	if (InBufferRHIRef->GetOwnerName() == NAME_None)
	{
		InBufferRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::SRVMask);

	return true;
}

bool UCompushadySRV::InitializeFromStructuredBuffer(FBufferRHIRef InBufferRHIRef)
{
	if (!InBufferRHIRef)
	{
		return false;
	}

	if (InBufferRHIRef->GetStride() == 0)
	{
		return false;
	}

	BufferRHIRef = InBufferRHIRef;

	ENQUEUE_RENDER_COMMAND(DoCompushadyCreateShaderResourceView)(
		[this](FRHICommandListImmediate& RHICmdList)
		{

			SRVRHIRef = COMPUSHADY_CREATE_SRV(BufferRHIRef);

		});

	FlushRenderingCommands();

	if (!SRVRHIRef)
	{
		return false;
	}

	if (InBufferRHIRef->GetOwnerName() == NAME_None)
	{
		InBufferRHIRef->SetOwnerName(*GetPathName());
	}

	RHITransitionInfo = FRHITransitionInfo(BufferRHIRef, ERHIAccess::Unknown, ERHIAccess::SRVMask);

	return true;
}

TPair<FShaderResourceViewRHIRef, FTextureRHIRef> UCompushadySRV::GetRHI(const FCompushadySceneTextures& SceneTextures) const
{
	return SceneTextures.Textures[(uint32)SceneTexture];
}

FShaderResourceViewRHIRef UCompushadySRV::GetRHI() const
{
	return SRVRHIRef;
}

bool UCompushadySRV::IsSceneTexture() const
{
	return SceneTexture != ECompushadySceneTexture::None;
}
