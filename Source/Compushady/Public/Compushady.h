// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCompushady, Log, All);

namespace Compushady
{
	enum class ECompushadySharedResourceType : uint8
	{
		UniformBuffer,
		Buffer,
		StructuredBuffer,
		ByteAddressBuffer,
		Texture
	};
	struct FCompushadyShaderResourceBinding
	{
		uint32 BindingIndex;
		uint32 SlotIndex;
		FString Name;
		ECompushadySharedResourceType Type;
	};

	struct FCompushadyShaderResourceBindings
	{
		TArray<FCompushadyShaderResourceBinding> CBVs;
		TArray<FCompushadyShaderResourceBinding> SRVs;
		TArray<FCompushadyShaderResourceBinding> UAVs;
	};

	bool CompileHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
	bool CompileGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FString& ErrorMessages);
	bool FixupSPIRV(TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
	bool FixupDXIL(TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages);
	bool DisassembleSPIRV(const TArray<uint8>& ByteCode, FString& Disassembled, FString& ErrorMessages);
	bool DisassembleDXIL(const TArray<uint8>& ByteCode, FString& Disassembled, FString& ErrorMessages);
	bool SPIRVToHLSL(const TArray<uint8>& ByteCode, FString& HLSL, FString& ErrorMessages);
	bool SPIRVToGLSL(const TArray<uint8>& ByteCode, FString& GLSL, FString& ErrorMessages);
	bool SPIRVToMSL(const TArray<uint8>& ByteCode, FString& MSL, FString& ErrorMessages);

	void DXCTeardown();
}

class FCompushadyModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
