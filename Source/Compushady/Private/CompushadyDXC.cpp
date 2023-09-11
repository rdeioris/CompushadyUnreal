// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include "Microsoft/COMPointer.h"
#endif

THIRD_PARTY_INCLUDES_START
#if PLATFORM_LINUX || PLATFORM_ANDROID || PLATFORM_MAC
#define __EMULATE_UUID
#endif

#if PLATFORM_ANDROID
#define SIZE_T SIZE_T_FAKE
#endif
#include "dxcapi.h"

#if PLATFORM_ANDROID
#undef SIZE_T
#endif

THIRD_PARTY_INCLUDES_END

#if PLATFORM_WINDOWS
#include <d3d12shader.h>
#endif

#if PLATFORM_WINDOWS
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if WITH_EDITOR
#include "Interfaces/IPluginManager.h"
#endif

#include "Serialization/ArrayWriter.h"

namespace Compushady
{
	namespace DXC
	{
		static void* LibHandle = nullptr;
		static void* DXILLibHandle = nullptr;
		static DxcCreateInstanceProc CreateInstance = nullptr;
		static IDxcLibrary* Library = nullptr;
		static IDxcCompiler3* Compiler = nullptr;
		static IDxcUtils* Utils = nullptr;
		static DxcCreateInstanceProc DxilCreateInstance = nullptr;
		static IDxcValidator* Validator = nullptr;

		static void Teardown()
		{
			if (Library)
			{
				Library->Release();
			}

			if (Compiler)
			{
				Compiler->Release();
			}

			if (Utils)
			{
				Utils->Release();
			}

			if (Validator)
			{
				Validator->Release();
			}

			if (DXILLibHandle)
			{
				FPlatformProcess::FreeDllHandle(DXILLibHandle);
			}

			if (LibHandle)
			{
				FPlatformProcess::FreeDllHandle(LibHandle);
			}
		}

		static bool Setup()
		{
			if (!LibHandle)
			{
#if PLATFORM_WINDOWS
#if WITH_EDITOR
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Compushady"))->GetBaseDir(), TEXT("Binaries/Win64/dxcompiler.dll"))));
#else
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectDir() / TEXT("Binaries/Win64/dxcompiler.dll")));
#endif
#elif PLATFORM_LINUX || PLATFORM_ANDROID
				LibHandle = FPlatformProcess::GetDllHandle(TEXT("libdxcompiler.so"));
#elif PLATFORM_MAC
				LibHandle = FPlatformProcess::GetDllHandle(TEXT("libdxcompiler.dylib"));
#endif
				if (!LibHandle)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to load dxcompiler shared library"));
					return false;
				}
			}

			if (!CreateInstance)
			{
				CreateInstance = reinterpret_cast<DxcCreateInstanceProc>(FPlatformProcess::GetDllExport(LibHandle, TEXT("DxcCreateInstance")));
				if (!CreateInstance)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find DxcCreateInstance symbol in dxcompiler"));
					return false;
				}
			}

			HRESULT HR;

			if (!Library)
			{
				HR = CreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), reinterpret_cast<void**>(&Library));
				if (!SUCCEEDED(HR))
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to create IDxcLibrary instance"));
					return false;
				}
			}

			if (!Compiler)
			{
				HR = CreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), reinterpret_cast<void**>(&Compiler));
				if (!SUCCEEDED(HR))
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to create IDxcCompiler3 instance"));
					return false;
				}
			}

			if (!Utils)
			{
				HR = CreateInstance(CLSID_DxcUtils, __uuidof(IDxcUtils), reinterpret_cast<void**>(&Utils));
				if (!SUCCEEDED(HR))
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to create IDxcUtils instance"));
					return false;
				}
			}

#if PLATFORM_WINDOWS
			if (!DXILLibHandle)
			{
#if WITH_EDITOR
				DXILLibHandle = FPlatformProcess::GetDllHandle(*(FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Compushady"))->GetBaseDir(), TEXT("Binaries/Win64/dxil.dll"))));
#else
				DXILLibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectDir() / TEXT("Binaries/Win64/dxil.dll")));
#endif
				if (!DXILLibHandle)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to load dxil shared library"));
					return false;
				}
			}

			if (!DxilCreateInstance)
			{
				DxilCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(FPlatformProcess::GetDllExport(DXILLibHandle, TEXT("DxcCreateInstance")));
				if (!DxilCreateInstance)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find DxcCreateInstance symbol in dxil"));
					return false;
				}
			}

			if (!Validator)
			{
				HR = DxilCreateInstance(CLSID_DxcValidator, __uuidof(IDxcValidator), reinterpret_cast<void**>(&Validator));
				if (!SUCCEEDED(HR))
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to create IDxcValidator instance"));
					return false;
				}
			}
#endif

			return true;
		}
	}
}

bool Compushady::DisassembleDXIL(const TArray<uint8>& ByteCode, FString& Disassembled, FString& ErrorMessages)
{
	if (ByteCode.Num() == 0)
	{
		ErrorMessages = "Empty ByteCode";
		return false;
	}

	if (!DXC::Setup())
	{
		ErrorMessages = "Failed DXCompiler initialization";
		return false;
	}

	DxcBuffer SourceBuffer = {};
	SourceBuffer.Ptr = ByteCode.GetData();
	SourceBuffer.Size = ByteCode.Num();

	IDxcResult* DisassembleResult;

	HRESULT HR = DXC::Compiler->Disassemble(&SourceBuffer, __uuidof(IDxcResult), reinterpret_cast<void**>(&DisassembleResult));
	if (!SUCCEEDED(HR))
	{
		ErrorMessages = "Unable to disassemble bytecode blob";
		return false;
	}

	DisassembleResult->GetStatus(&HR);

	if (!SUCCEEDED(HR))
	{
		IDxcBlobEncoding* BlobError;
		HR = DisassembleResult->GetErrorBuffer(&BlobError);
		if (SUCCEEDED(HR))
		{
			ErrorMessages = FString(BlobError->GetBufferSize(), reinterpret_cast<const char*>(BlobError->GetBufferPointer()));
			BlobError->Release();
		}
		else
		{
			ErrorMessages = "Unable to disassemble code blob";
		}

		DisassembleResult->Release();
		return false;
	}

	IDxcBlob* DisassembledBlob;
	HR = DisassembleResult->GetResult(&DisassembledBlob);
	if (!SUCCEEDED(HR))
	{
		ErrorMessages = "Unable to get disassemble result";
		return false;
	}

	Disassembled = FString(DisassembledBlob->GetBufferSize(), reinterpret_cast<const char*>(DisassembledBlob->GetBufferPointer()));

	DisassembledBlob->Release();
	DisassembleResult->Release();

	return true;
}

bool Compushady::CompileHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{

	if (ShaderCode.Num() == 0)
	{
		ErrorMessages = "Empty ShaderCode";
		return false;
	}

	if (EntryPoint.IsEmpty())
	{
		ErrorMessages = "Empty EntryPoint";
		return false;
	}

	if (TargetProfile.IsEmpty())
	{
		ErrorMessages = "Empty TargetProfile";
		return false;
	}

	if (!DXC::Setup())
	{
		ErrorMessages = "Failed DXCompiler initialization";
		return false;
	}

	HRESULT HR;

	IDxcBlobEncoding* BlobSource;

	HR = DXC::Library->CreateBlobWithEncodingOnHeapCopy(ShaderCode.GetData(), ShaderCode.Num(), DXC_CP_UTF8, &BlobSource);
	if (!SUCCEEDED(HR))
	{
		ErrorMessages = "Unable to create code blob";
		return false;
	}

	ERHIInterfaceType RHIInterfaceType = RHIGetInterfaceType();

	TArray<LPCWSTR> Arguments;

	FTCHARToWChar WideTargetProfile(*TargetProfile);
	FTCHARToWChar WideEntryPoint(*EntryPoint);

	Arguments.Add(L"-T");
	Arguments.Add(WideTargetProfile.Get());

	Arguments.Add(L"-E");
	Arguments.Add(WideEntryPoint.Get());

	// compile to spirv
	if (RHIInterfaceType == ERHIInterfaceType::Vulkan || RHIInterfaceType == ERHIInterfaceType::Metal)
	{
		Arguments.Add(L"-spirv");
		Arguments.Add(L"-fvk-use-dx-layout");
		Arguments.Add(L"-fvk-use-scalar-layout");
		Arguments.Add(L"-fspv-entrypoint-name=main_00000000_00000000");
		Arguments.Add(L"-fspv-reflect");
	}

	DxcBuffer SourceBuffer;
	SourceBuffer.Ptr = BlobSource->GetBufferPointer();
	SourceBuffer.Size = BlobSource->GetBufferSize();
	SourceBuffer.Encoding = 0;

	IDxcResult* CompileResult = nullptr;
	HR = DXC::Compiler->Compile(&SourceBuffer, Arguments.GetData(), Arguments.Num(), nullptr, __uuidof(IDxcResult), reinterpret_cast<void**>(&CompileResult));
	if (!SUCCEEDED(HR))
	{
		ErrorMessages = "Unable to compile code blob";
		BlobSource->Release();
		return false;
	}

	BlobSource->Release();

	CompileResult->GetStatus(&HR);

	if (!SUCCEEDED(HR))
	{
		IDxcBlobEncoding* BlobError;
		HR = CompileResult->GetErrorBuffer(&BlobError);
		if (SUCCEEDED(HR))
		{
			ErrorMessages = FString(BlobError->GetBufferSize(), reinterpret_cast<const char*>(BlobError->GetBufferPointer()));
			BlobError->Release();
		}
		else
		{
			ErrorMessages = "Unable to compile code blob";
		}

		CompileResult->Release();
		return false;
	}

	IDxcBlob* CompiledBlob;
	HR = CompileResult->GetResult(&CompiledBlob);
	if (!SUCCEEDED(HR))
	{
		ErrorMessages = "Unable to get compile result";
		return false;
	}

	CompileResult->Release();

	// validate the shader
	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
#if PLATFORM_WINDOWS
		IDxcOperationResult* VerifyResult;
		DXC::Validator->Validate(CompiledBlob, DxcValidatorFlags_InPlaceEdit, &VerifyResult);
		if (!SUCCEEDED(HR) || !VerifyResult)
		{
			ErrorMessages = "Unable to validate shader";
			CompiledBlob->Release();
			return false;
		}
		VerifyResult->Release();
#endif
	}

	ByteCode.Append(reinterpret_cast<const uint8*>(CompiledBlob->GetBufferPointer()), CompiledBlob->GetBufferSize());

	CompiledBlob->Release();

	if (RHIInterfaceType == ERHIInterfaceType::Vulkan)
	{
		if (!FixupSPIRV(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
		{
			return false;
		}
	}
	else if (RHIInterfaceType == ERHIInterfaceType::Metal)
	{
		// TODO convert to MSL
		return false;
	}
	else if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{
		if (!FixupDXIL(ByteCode, ShaderResourceBindings, ThreadGroupSize, ErrorMessages))
		{
			return false;
		}
	}

	return true;
}

bool Compushady::FixupDXIL(TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	if (!DXC::Setup())
	{
		ErrorMessages = "Failed DXCompiler initialization";
		return false;
	}

#if PLATFORM_WINDOWS
	TMap<uint32, FCompushadyShaderResourceBinding> CBVMapping;
	TMap<uint32, FCompushadyShaderResourceBinding> SRVMapping;
	TMap<uint32, FCompushadyShaderResourceBinding> UAVMapping;

	ID3D12ShaderReflection* ShaderReflection;

	DxcBuffer ReflectionBuffer;
	ReflectionBuffer.Ptr = ByteCode.GetData();
	ReflectionBuffer.Size = ByteCode.Num();
	ReflectionBuffer.Encoding = 0;
	HRESULT HR = DXC::Utils->CreateReflection(&ReflectionBuffer, __uuidof(ID3D12ShaderReflection), reinterpret_cast<void**>(&ShaderReflection));

	if (!SUCCEEDED(HR))
	{
		ErrorMessages = "Unable to create reflection";
		return false;
	}

	D3D12_SHADER_DESC ShaderDesc;
	ShaderReflection->GetDesc(&ShaderDesc);

	/*
		D3D_SIT_CBUFFER
		D3D_SIT_TBUFFER
		D3D_SIT_TEXTURE
		D3D_SIT_SAMPLER
		D3D_SIT_UAV_RWTYPED
		D3D_SIT_STRUCTURED
		D3D_SIT_UAV_RWSTRUCTURED
		D3D_SIT_BYTEADDRESS
		D3D_SIT_UAV_RWBYTEADDRESS
		D3D_SIT_UAV_APPEND_STRUCTURED
		D3D_SIT_UAV_CONSUME_STRUCTURED
		D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER
		D3D_SIT_RTACCELERATIONSTRUCTURE
		D3D_SIT_UAV_FEEDBACKTEXTURE
	*/

	for (uint32 Index = 0; Index < ShaderDesc.BoundResources; Index++)
	{
		D3D12_SHADER_INPUT_BIND_DESC BindDesc;
		ShaderReflection->GetResourceBindingDesc(Index, &BindDesc);

		FCompushadyShaderResourceBinding ResourceBinding;
		ResourceBinding.BindingIndex = BindDesc.BindPoint;
		ResourceBinding.SlotIndex = ResourceBinding.BindingIndex;
		ResourceBinding.Name = BindDesc.Name;

		switch (BindDesc.Type)
		{
		case D3D_SIT_CBUFFER:
			ResourceBinding.Type = ECompushadySharedResourceType::UniformBuffer;
			CBVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case D3D_SIT_TEXTURE:
			ResourceBinding.Type = BindDesc.Dimension == D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER ? ECompushadySharedResourceType::Buffer : ECompushadySharedResourceType::Texture;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case D3D_SIT_BYTEADDRESS:
			ResourceBinding.Type = ECompushadySharedResourceType::ByteAddressBuffer;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case D3D_SIT_STRUCTURED:
			ResourceBinding.Type = ECompushadySharedResourceType::StructuredBuffer;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case D3D_SIT_TBUFFER:
			ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case D3D_SIT_UAV_RWTYPED:
			ResourceBinding.Type = BindDesc.Dimension == D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER ? ECompushadySharedResourceType::Buffer : ECompushadySharedResourceType::Texture;
			UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case D3D_SIT_UAV_FEEDBACKTEXTURE:
			ResourceBinding.Type = ECompushadySharedResourceType::Texture;
			UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			ResourceBinding.Type = ECompushadySharedResourceType::StructuredBuffer;
			UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case D3D_SIT_UAV_RWBYTEADDRESS:
			ResourceBinding.Type = ECompushadySharedResourceType::ByteAddressBuffer;
			UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		default:
			ErrorMessages = FString::Printf(TEXT("Unsupported resource type for %s"), UTF8_TO_TCHAR(BindDesc.Name));
			ShaderReflection->Release();
			return false;
		}

	}

	UINT TGX, TGY, TGZ;
	ShaderReflection->GetThreadGroupSize(&TGX, &TGY, &TGZ);
	ThreadGroupSize = FIntVector(TGX, TGY, TGZ);

	ShaderReflection->Release();

	FShaderResourceTable ShaderResourceTable;
	FArrayWriter Writer;
	Writer << ShaderResourceTable;

	ByteCode.Insert(Writer, 0);

	// sort resources
	TArray<uint32> CBVKeys;
	CBVMapping.GetKeys(CBVKeys);
	CBVKeys.Sort();

	for (uint32 CBVIndex : CBVKeys)
	{
		ShaderResourceBindings.CBVs.Add(CBVMapping[CBVIndex]);
	}

	TArray<uint32> SRVKeys;
	SRVMapping.GetKeys(SRVKeys);
	SRVKeys.Sort();

	for (uint32 SRVIndex : SRVKeys)
	{
		ShaderResourceBindings.SRVs.Add(SRVMapping[SRVIndex]);
	}

	TArray<uint32> UAVKeys;
	UAVMapping.GetKeys(UAVKeys);
	UAVKeys.Sort();

	for (uint32 UAVIndex : UAVKeys)
	{
		ShaderResourceBindings.UAVs.Add(UAVMapping[UAVIndex]);
	}

#endif
	return true;
}

void Compushady::DXCTeardown()
{
	Compushady::DXC::Teardown();
}
