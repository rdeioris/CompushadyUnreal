// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include "Windows/WindowsPlatformProcess.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#if COMPUSHADY_UE_VERSION <= 53
#include "Windows/PreWindowsApi.h"
#endif
#include "Microsoft/COMPointer.h"
#endif

#include "Misc/Paths.h"
#include "RHIDefinitions.h"
#include "RHIResources.h"
#include "DynamicRHI.h"

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

// this is a pretty annoying hack for avoiding conflict between unreal d3dcommon.h and the system one
enum _COMPUSHADY_D3D_SHADER_INPUT_TYPE
{
	COMPUSHADY_D3D_SIT_CBUFFER = 0,
	COMPUSHADY_D3D_SIT_TBUFFER = (COMPUSHADY_D3D_SIT_CBUFFER + 1),
	COMPUSHADY_D3D_SIT_TEXTURE = (COMPUSHADY_D3D_SIT_TBUFFER + 1),
	COMPUSHADY_D3D_SIT_SAMPLER = (COMPUSHADY_D3D_SIT_TEXTURE + 1),
	COMPUSHADY_D3D_SIT_UAV_RWTYPED = (COMPUSHADY_D3D_SIT_SAMPLER + 1),
	COMPUSHADY_D3D_SIT_STRUCTURED = (COMPUSHADY_D3D_SIT_UAV_RWTYPED + 1),
	COMPUSHADY_D3D_SIT_UAV_RWSTRUCTURED = (COMPUSHADY_D3D_SIT_STRUCTURED + 1),
	COMPUSHADY_D3D_SIT_BYTEADDRESS = (COMPUSHADY_D3D_SIT_UAV_RWSTRUCTURED + 1),
	COMPUSHADY_D3D_SIT_UAV_RWBYTEADDRESS = (COMPUSHADY_D3D_SIT_BYTEADDRESS + 1),
	COMPUSHADY_D3D_SIT_UAV_APPEND_STRUCTURED = (COMPUSHADY_D3D_SIT_UAV_RWBYTEADDRESS + 1),
	COMPUSHADY_D3D_SIT_UAV_CONSUME_STRUCTURED = (COMPUSHADY_D3D_SIT_UAV_APPEND_STRUCTURED + 1),
	COMPUSHADY_D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER = (COMPUSHADY_D3D_SIT_UAV_CONSUME_STRUCTURED + 1),
	COMPUSHADY_D3D_SIT_RTACCELERATIONSTRUCTURE = (COMPUSHADY_D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER + 1),
	COMPUSHADY_D3D_SIT_UAV_FEEDBACKTEXTURE = (COMPUSHADY_D3D_SIT_RTACCELERATIONSTRUCTURE + 1),
};
#endif

#if PLATFORM_WINDOWS
#if COMPUSHADY_UE_VERSION <= 53
#include "Windows/PostWindowsApi.h"
#endif
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
#elif PLATFORM_LINUX
#if WITH_EDITOR
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Compushady"))->GetBaseDir(), TEXT("Binaries/Linux/libdxcompiler.so"))));
#else
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectDir() / TEXT("Binaries/Linux/libdxcompiler.so")));
#endif
#elif PLATFORM_ANDROID
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

bool Compushady::CompileHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FString& ErrorMessages, const bool bForceSPIRV)
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
	if (RHIInterfaceType == ERHIInterfaceType::Vulkan || RHIInterfaceType == ERHIInterfaceType::Metal || bForceSPIRV)
	{
		Arguments.Add(L"-spirv");
		Arguments.Add(L"-fvk-use-dx-layout");
		Arguments.Add(L"-fvk-use-scalar-layout");
		Arguments.Add(L"-fspv-entrypoint-name=main_00000000_00000000");
		Arguments.Add(L"-fspv-reflect");
		if (TargetProfile.StartsWith("lib_"))
		{
			Arguments.Add(L"-fspv-target-env=vulkan1.2");
		}
	}

	DxcBuffer SourceBuffer;
	SourceBuffer.Ptr = BlobSource->GetBufferPointer();
	SourceBuffer.Size = BlobSource->GetBufferSize();
	SourceBuffer.Encoding = 0;

	struct ICompushadyNopIncludeHandler : public IDxcIncludeHandler
	{
		HRESULT STDMETHODCALLTYPE LoadSource(_In_z_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
		{
			UE_LOG(LogCompushady, Error, TEXT("#include directives are disabled!"));
			*ppIncludeSource = nullptr;
			return E_NOTIMPL;
		}

#if PLATFORM_WINDOWS
		HRESULT STDMETHODCALLTYPE QueryInterface(/* [in] */ REFIID riid,
			/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override {
			return E_NOINTERFACE;
		}
#else
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
		{
			return E_NOINTERFACE;
		}
#endif
		ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
		ULONG STDMETHODCALLTYPE Release(void) override { return 0; }
	};

	ICompushadyNopIncludeHandler NopIncludeHandler;

	IDxcResult* CompileResult = nullptr;
	HR = DXC::Compiler->Compile(&SourceBuffer, Arguments.GetData(), Arguments.Num(), &NopIncludeHandler, __uuidof(IDxcResult), reinterpret_cast<void**>(&CompileResult));
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
	if (RHIInterfaceType == ERHIInterfaceType::D3D12 && !bForceSPIRV)
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
	TMap<uint32, FCompushadyShaderResourceBinding> SamplerMapping;

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

	for (uint32 Index = 0; Index < ShaderDesc.InputParameters; Index++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC SignatureDesc;
		ShaderReflection->GetInputParameterDesc(Index, &SignatureDesc);
		if (SignatureDesc.SystemValueType == D3D_NAME_UNDEFINED)
		{
			ShaderResourceBindings.InputSemantics.Add(FCompushadyShaderSemantic(UTF8_TO_TCHAR(SignatureDesc.SemanticName), SignatureDesc.SemanticIndex, SignatureDesc.Register, SignatureDesc.Mask));
		}
	}

	for (uint32 Index = 0; Index < ShaderDesc.OutputParameters; Index++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC SignatureDesc;
		ShaderReflection->GetOutputParameterDesc(Index, &SignatureDesc);
		if (SignatureDesc.SystemValueType == D3D_NAME_UNDEFINED)
		{
			ShaderResourceBindings.OutputSemantics.Add(FCompushadyShaderSemantic(UTF8_TO_TCHAR(SignatureDesc.SemanticName), SignatureDesc.SemanticIndex, SignatureDesc.Register, SignatureDesc.Mask));
		}
	}

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
		case COMPUSHADY_D3D_SIT_CBUFFER:
			ResourceBinding.Type = ECompushadyShaderResourceType::UniformBuffer;
			CBVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_TEXTURE:
			ResourceBinding.Type = BindDesc.Dimension == D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER ? ECompushadyShaderResourceType::Buffer : ECompushadyShaderResourceType::Texture;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_SAMPLER:
			ResourceBinding.Type = ECompushadyShaderResourceType::Sampler;
			SamplerMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_BYTEADDRESS:
			ResourceBinding.Type = ECompushadyShaderResourceType::ByteAddressBuffer;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_STRUCTURED:
			ResourceBinding.Type = ECompushadyShaderResourceType::StructuredBuffer;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_TBUFFER:
			ResourceBinding.Type = ECompushadyShaderResourceType::Buffer;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_RTACCELERATIONSTRUCTURE:
			ResourceBinding.Type = ECompushadyShaderResourceType::RayTracingAccelerationStructure;
			SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_UAV_RWTYPED:
			ResourceBinding.Type = BindDesc.Dimension == D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER ? ECompushadyShaderResourceType::Buffer : ECompushadyShaderResourceType::Texture;
			UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_UAV_FEEDBACKTEXTURE:
			ResourceBinding.Type = ECompushadyShaderResourceType::Texture;
			UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_UAV_RWSTRUCTURED:
		case COMPUSHADY_D3D_SIT_UAV_APPEND_STRUCTURED:
		case COMPUSHADY_D3D_SIT_UAV_CONSUME_STRUCTURED:
		case COMPUSHADY_D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			ResourceBinding.Type = ECompushadyShaderResourceType::StructuredBuffer;
			UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
			break;
		case COMPUSHADY_D3D_SIT_UAV_RWBYTEADDRESS:
			ResourceBinding.Type = ECompushadyShaderResourceType::ByteAddressBuffer;
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

	TArray<uint32> SamplerKeys;
	SamplerMapping.GetKeys(SamplerKeys);
	SamplerKeys.Sort();

	for (uint32 SamplerIndex : SamplerKeys)
	{
		ShaderResourceBindings.Samplers.Add(SamplerMapping[SamplerIndex]);
	}

#endif
	return true;
}

void Compushady::DXCTeardown()
{
	Compushady::DXC::Teardown();
}
