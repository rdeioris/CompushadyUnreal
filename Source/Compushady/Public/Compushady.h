// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Launch/Resources/Version.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCompushady, Log, All);

#if ENGINE_MAJOR_VERSION == 5
#if ENGINE_MINOR_VERSION == 2
#define COMPUSHADY_UE_VERSION 52
#define COMPUSHADY_CREATE_BUFFER(Name, Size, Flags, Stride, InitialState) [&]() { FRHIResourceCreateInfo ResourceCreateInfo(*Name); return RHICreateBuffer(Size, Flags, Stride, InitialState, ResourceCreateInfo);}()
#define COMPUSHADY_CREATE_SRV RHICreateShaderResourceView
#define COMPUSHADY_CREATE_UAV RHICreateUnorderedAccessView
#elif ENGINE_MINOR_VERSION == 3
#define COMPUSHADY_UE_VERSION 53
#define COMPUSHADY_CREATE_BUFFER(Name, Size, Flags, Stride, InitialState) [&]() { FRHIResourceCreateInfo ResourceCreateInfo(*Name); return RHICmdList.CreateBuffer(Size, Flags, Stride, InitialState, ResourceCreateInfo);}()
#define COMPUSHADY_CREATE_SRV RHICmdList.CreateShaderResourceView
#define COMPUSHADY_CREATE_UAV RHICmdList.CreateUnorderedAccessView
#elif ENGINE_MINOR_VERSION == 4
#define COMPUSHADY_UE_VERSION 54
#define COMPUSHADY_CREATE_BUFFER(Name, Size, Flags, Stride, InitialState) [&]() { FRHIResourceCreateInfo ResourceCreateInfo(*Name); return RHICmdList.CreateBuffer(Size, Flags, Stride, InitialState, ResourceCreateInfo);}()
#define COMPUSHADY_CREATE_SRV RHICmdList.CreateShaderResourceView
#define COMPUSHADY_CREATE_UAV RHICmdList.CreateUnorderedAccessView
#elif ENGINE_MINOR_VERSION == 5
#define COMPUSHADY_UE_VERSION 55
#define COMPUSHADY_CREATE_BUFFER(Name, Size, Flags, Stride, InitialState) [&]() { FRHIResourceCreateInfo ResourceCreateInfo(*Name); return RHICmdList.CreateBuffer(Size, Flags, Stride, InitialState, ResourceCreateInfo);}()
#define COMPUSHADY_CREATE_SRV RHICmdList.CreateShaderResourceView
#define COMPUSHADY_CREATE_UAV RHICmdList.CreateUnorderedAccessView
#elif ENGINE_MINOR_VERSION == 6
#define COMPUSHADY_UE_VERSION 56
#define COMPUSHADY_CREATE_BUFFER(Name, Size, Flags, Stride, InitialState) [&]() { FRHIBufferCreateDesc ResourceCreateInfo = FRHIBufferCreateDesc::Create(Name, Size, Stride, Flags).SetInitialState(InitialState); return RHICmdList.CreateBuffer(ResourceCreateInfo);}()
#define COMPUSHADY_CREATE_SRV RHICmdList.CreateShaderResourceView
#define COMPUSHADY_CREATE_UAV RHICmdList.CreateUnorderedAccessView
#endif
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
	COMPUSHADY_API bool ToUnrealShader(const TArray<uint8>& ByteCode, TArray<uint8>& Blob, const uint32 NumCBVs, const uint32 NumSRVs, const uint32 NumUAVs, const uint32 NumSamplers, FSHAHash& Hash);
	COMPUSHADY_API FSHAHash GetHash(const TArrayView<uint8>& Data);

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
