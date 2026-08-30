// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Launch/Resources/Version.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCompushady, Log, All);

// ------------------------------------------------------------------------------------------------
// Cross-version RHI shims.
//
// COMPUSHADY_UE_VERSION is the engine version as a two-digit number (5.7 -> 57). Resource creation
// changed shape a few times, so the create helpers below expand differently per engine "era":
//   * 5.2            : global RHICreate* entry points, loose-argument view/buffer creation.
//   * 5.3 - 5.5      : the same loose-argument creation, but moved onto the command list (RHICmdList).
//   * 5.6 and later  : buffers use FRHIBufferCreateDesc, and the loose-argument CreateShaderResourceView
//                      / CreateUnorderedAccessView overloads were deprecated in favour of ones taking an
//                      FRHIViewDesc. From 5.6 we therefore build the FRHIViewDesc explicitly (mirroring
//                      exactly what the engine's now-deprecated back-compat wrappers did), which is what
//                      silences the 5.6/5.7/5.8 deprecation warnings.
//
// The SRV/UAV creators are split per view shape (whole texture, texture-from-CreateInfo, pre-built
// view desc, typed buffer, structured/raw buffer) because the modern API needs a distinct FRHIViewDesc
// for each. All command-list forms expect a variable named RHICmdList to be in scope at the call site.
// ------------------------------------------------------------------------------------------------
#if ENGINE_MAJOR_VERSION == 5
#if ENGINE_MINOR_VERSION == 2
#define COMPUSHADY_UE_VERSION 52
#elif ENGINE_MINOR_VERSION == 3
#define COMPUSHADY_UE_VERSION 53
#elif ENGINE_MINOR_VERSION == 4
#define COMPUSHADY_UE_VERSION 54
#elif ENGINE_MINOR_VERSION == 5
#define COMPUSHADY_UE_VERSION 55
#elif ENGINE_MINOR_VERSION == 6
#define COMPUSHADY_UE_VERSION 56
#elif ENGINE_MINOR_VERSION == 7
#define COMPUSHADY_UE_VERSION 57
#elif ENGINE_MINOR_VERSION == 8
#define COMPUSHADY_UE_VERSION 58
#endif
#endif

// ---- shader identity hash ----
//
// 5.8 replaced FSHAHash (160-bit SHA-1) with FShaderHash (64-bit xxhash) as the shader
// identity type taken by RHICreate*Shader and FRHIShader::SetHash. Compushady only ever
// computes this hash from a finished shader blob and hands it straight to the RHI - it is
// never stored, serialised, or compared - so the change of algorithm is transparent here.
#if COMPUSHADY_UE_VERSION >= 58
#include "Hash/ShaderHash.h"
#include "Hash/xxhash.h"
using FCompushadyShaderHash = FShaderHash;
#else
#include "Misc/SecureHash.h"
using FCompushadyShaderHash = FSHAHash;
#endif

// ---- buffer creation ----
#if COMPUSHADY_UE_VERSION == 52
#define COMPUSHADY_CREATE_BUFFER(Name, Size, Flags, Stride, InitialState) [&]() { FRHIResourceCreateInfo ResourceCreateInfo(*Name); return RHICreateBuffer(Size, Flags, Stride, InitialState, ResourceCreateInfo);}()
#elif COMPUSHADY_UE_VERSION >= 53 && COMPUSHADY_UE_VERSION <= 55
#define COMPUSHADY_CREATE_BUFFER(Name, Size, Flags, Stride, InitialState) [&]() { FRHIResourceCreateInfo ResourceCreateInfo(*Name); return RHICmdList.CreateBuffer(Size, Flags, Stride, InitialState, ResourceCreateInfo);}()
#elif COMPUSHADY_UE_VERSION >= 56
#define COMPUSHADY_CREATE_BUFFER(Name, Size, Flags, Stride, InitialState) [&]() { FRHIBufferCreateDesc ResourceCreateInfo = FRHIBufferCreateDesc::Create(Name, Size, Stride, Flags).SetInitialState(InitialState); return RHICmdList.CreateBuffer(ResourceCreateInfo);}()
#endif

// ---- SRV / UAV creation ----
#if COMPUSHADY_UE_VERSION == 52
#define COMPUSHADY_CREATE_SRV_TEXTURE(Texture)                     RHICreateShaderResourceView(Texture, 0)
#define COMPUSHADY_CREATE_SRV_TEXTURE_INFO(Texture, Info)          RHICreateShaderResourceView(Texture, Info)
#define COMPUSHADY_CREATE_SRV_TEXTURE_VIEWDESC(Texture, ViewDesc)  RHICreateShaderResourceView(Texture, ViewDesc)
#define COMPUSHADY_CREATE_SRV_BUFFER_TYPED(Buffer, Stride, Format) RHICreateShaderResourceView(Buffer, Stride, Format)
#define COMPUSHADY_CREATE_SRV_BUFFER(Buffer)                       RHICreateShaderResourceView(Buffer)
#define COMPUSHADY_CREATE_UAV_TEXTURE(Texture)                     RHICreateUnorderedAccessView(Texture)
#define COMPUSHADY_CREATE_UAV_BUFFER_TYPED(Buffer, Format)         RHICreateUnorderedAccessView(Buffer, Format)
#define COMPUSHADY_CREATE_UAV_BUFFER(Buffer)                       RHICreateUnorderedAccessView(Buffer, false, false)
#elif COMPUSHADY_UE_VERSION >= 53 && COMPUSHADY_UE_VERSION <= 55
#define COMPUSHADY_CREATE_SRV_TEXTURE(Texture)                     RHICmdList.CreateShaderResourceView(Texture, 0)
#define COMPUSHADY_CREATE_SRV_TEXTURE_INFO(Texture, Info)          RHICmdList.CreateShaderResourceView(Texture, Info)
#define COMPUSHADY_CREATE_SRV_TEXTURE_VIEWDESC(Texture, ViewDesc)  RHICmdList.CreateShaderResourceView(Texture, ViewDesc)
#define COMPUSHADY_CREATE_SRV_BUFFER_TYPED(Buffer, Stride, Format) RHICmdList.CreateShaderResourceView(Buffer, Stride, Format)
#define COMPUSHADY_CREATE_SRV_BUFFER(Buffer)                       RHICmdList.CreateShaderResourceView(Buffer)
#define COMPUSHADY_CREATE_UAV_TEXTURE(Texture)                     RHICmdList.CreateUnorderedAccessView(Texture)
#define COMPUSHADY_CREATE_UAV_BUFFER_TYPED(Buffer, Format)         RHICmdList.CreateUnorderedAccessView(Buffer, Format)
#define COMPUSHADY_CREATE_UAV_BUFFER(Buffer)                       RHICmdList.CreateUnorderedAccessView(Buffer, false, false)
#elif COMPUSHADY_UE_VERSION >= 56
// FRHIViewDesc-based creation (loose-argument overloads deprecated in 5.6). These mirror the engine's
// own back-compat wrappers so behaviour is identical, but call the non-deprecated overloads directly.
#define COMPUSHADY_CREATE_SRV_TEXTURE(Texture)                     RHICmdList.CreateShaderResourceView((Texture), FRHIViewDesc::CreateTextureSRV().SetDimensionFromTexture(Texture).SetMipRange(0, 1))
#define COMPUSHADY_CREATE_SRV_TEXTURE_INFO(Texture, Info)          RHICmdList.CreateShaderResourceView((Texture), FRHIViewDesc::CreateTextureSRV().SetDimensionFromTexture(Texture).SetFormat((Info).Format).SetMipRange((Info).MipLevel, (Info).NumMipLevels).SetDisableSRGB((Info).SRGBOverride == SRGBO_ForceDisable).SetArrayRange((Info).FirstArraySlice, (Info).NumArraySlices).SetPlane((Info).MetaData))
#define COMPUSHADY_CREATE_SRV_TEXTURE_VIEWDESC(Texture, ViewDesc)  RHICmdList.CreateShaderResourceView((Texture), ViewDesc)
#define COMPUSHADY_CREATE_SRV_BUFFER_TYPED(Buffer, Stride, Format) RHICmdList.CreateShaderResourceView((Buffer), FRHIViewDesc::CreateBufferSRV().SetType(FRHIViewDesc::EBufferType::Typed).SetFormat(EPixelFormat(Format)))
#define COMPUSHADY_CREATE_SRV_BUFFER(Buffer)                       RHICmdList.CreateShaderResourceView((Buffer), FRHIViewDesc::CreateBufferSRV().SetTypeFromBuffer(Buffer))
#define COMPUSHADY_CREATE_UAV_TEXTURE(Texture)                     RHICmdList.CreateUnorderedAccessView((Texture), FRHIViewDesc::CreateTextureUAV().SetDimensionFromTexture(Texture).SetMipLevel(0))
#define COMPUSHADY_CREATE_UAV_BUFFER_TYPED(Buffer, Format)         RHICmdList.CreateUnorderedAccessView((Buffer), FRHIViewDesc::CreateBufferUAV().SetType(FRHIViewDesc::EBufferType::Typed).SetFormat(EPixelFormat(Format)))
#define COMPUSHADY_CREATE_UAV_BUFFER(Buffer)                       RHICmdList.CreateUnorderedAccessView((Buffer), FRHIViewDesc::CreateBufferUAV().SetTypeFromBuffer(Buffer).SetAtomicCounter(false).SetAppendBuffer(false))
#endif

#if PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_WINDOWS
#define COMPUSHADY_SUPPORTS_VIDEO_ENCODING
#endif

namespace Compushady
{
	enum class ECompushadyShaderResourceType : uint8
	{
		UniformBuffer,
		Buffer,
		StructuredBuffer,
		ByteAddressBuffer,
		Texture,
		Sampler,
		RayTracingAccelerationStructure
	};

	struct FCompushadyShaderResourceBinding
	{
		uint32 BindingIndex;
		uint32 SlotIndex;
		FString Name;
		ECompushadyShaderResourceType Type;
	};

	struct FCompushadyShaderSemantic
	{
		FString Name;
		uint32 Index;
		uint32 Register;
		uint32 Mask;

		bool operator==(const FCompushadyShaderSemantic& Other) const
		{
			return Name == Other.Name && Index == Other.Index && Register == Other.Register && Mask == Other.Mask;
		}

		FCompushadyShaderSemantic(const FString& InName, const uint32 InIndex, const uint32 InRegister, const uint32 InMask)
		{
			Name = InName;
			Index = InIndex;
			Register = InRegister;
			Mask = InMask;
		}
	};

	struct FCompushadyShaderResourceBindings
	{
		TArray<FCompushadyShaderResourceBinding> CBVs;
		TArray<FCompushadyShaderResourceBinding> SRVs;
		TArray<FCompushadyShaderResourceBinding> UAVs;
		TArray<FCompushadyShaderResourceBinding> Samplers;

		TArray<FCompushadyShaderSemantic> InputSemantics;
		TArray<FCompushadyShaderSemantic> OutputSemantics;
	};

	COMPUSHADY_API bool CompileHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FString& ErrorMessages, const bool bForceSPIRV);
	COMPUSHADY_API bool CompileGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FString& ErrorMessages);
	COMPUSHADY_API bool FixupSPIRV(TArray<uint8>& ByteCode, const FString& TargetProfile, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
	COMPUSHADY_API bool FixupDXIL(TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
	COMPUSHADY_API bool DisassembleSPIRV(const TArray<uint8>& ByteCode, TArray<uint8>& Disassembled, FString& ErrorMessages);
	COMPUSHADY_API bool DisassembleDXIL(const TArray<uint8>& ByteCode, FString& Disassembled, FString& ErrorMessages);
	COMPUSHADY_API bool SPIRVToHLSL(const TArray<uint8>& ByteCode, TArray<uint8>& HLSL, FString& EntryPoint, FString& ErrorMessages);
	COMPUSHADY_API bool SPIRVToGLSL(const TArray<uint8>& ByteCode, TArray<uint8>& GLSL, FString& ErrorMessages);
	COMPUSHADY_API bool SPIRVToMSL(const TArray<uint8>& ByteCode, FString& MSL, FString& ErrorMessages);

	COMPUSHADY_API void StringToShaderCode(const FString& Code, TArray<uint8>& ShaderCode);
	COMPUSHADY_API void StringToCString(const FString& Code, TArray<uint8>& ShaderCode);
	COMPUSHADY_API FString ShaderCodeToString(const TArray<uint8>& ShaderCode);
	COMPUSHADY_API bool ToUnrealShader(const TArray<uint8>& ByteCode, TArray<uint8>& Blob, const uint32 NumCBVs, const uint32 NumSRVs, const uint32 NumUAVs, const uint32 NumSamplers, FCompushadyShaderHash& Hash);
	COMPUSHADY_API FCompushadyShaderHash GetHash(const TArrayView<uint8>& Data);

	COMPUSHADY_API bool FileToByteArray(const FString& Filename, const bool bRelativeToContent, TArray<uint8>& Bytes);

	void DXCTeardown();
}

class FCompushadyModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
