// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

#define LOCTEXT_NAMESPACE "FCompushadyModule"

DEFINE_LOG_CATEGORY(LogCompushady);

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include "Microsoft/COMPointer.h"
#endif

#include "dxcapi.h"
#if PLATFORM_WINDOWS
#include <d3d12shader.h>
#endif

#if PLATFORM_WINDOWS
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif


#define LOCTEXT_NAMESPACE "FCompushadyModule"

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
				LibHandle = FPlatformProcess::GetDllHandle(TEXT("dxcompiler.dll"));
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
				DXILLibHandle = FPlatformProcess::GetDllHandle(TEXT("dxil.dll"));
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


bool Compushady::CompileHLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FString& ErrorMessages)
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

	if (!DXC::Setup())
	{
		ErrorMessages = "Failed DXCompiler initialization";
		return false;
	}

	HRESULT HR;

	IDxcBlobEncoding* BlobSource;

	//hr = DXCLibrary->CreateBlobWithEncodingOnHeapCopy(ShaderSourceBytes.GetData(), ShaderSourceBytes.Num() * sizeof(TCHAR), DXC_CP_UTF16, &BlobSource);

	HR = DXC::Library->CreateBlobWithEncodingOnHeapCopy(ShaderCode.GetData(), ShaderCode.Num(), DXC_CP_UTF8, &BlobSource);
	if (!SUCCEEDED(HR))
	{
		ErrorMessages = "Unable to create code blob";
		return false;
	}

	TArray<LPCWSTR> Arguments;
	ERHIInterfaceType RHIInterfaceType = RHIGetInterfaceType();

	Arguments.Add(TEXT("-T"));
	Arguments.Add(*TargetProfile);

	Arguments.Add(TEXT("-E"));
	Arguments.Add(*EntryPoint);

	// compile to spirv
	if (RHIInterfaceType == ERHIInterfaceType::Vulkan || RHIInterfaceType == ERHIInterfaceType::Metal)
	{
		Arguments.Add(TEXT("-spirv"));
		Arguments.Add(TEXT("-fvk-t-shift"));
		Arguments.Add(TEXT("1024"));
		Arguments.Add(TEXT("0"));
		Arguments.Add(TEXT("-fvk-u-shift"));
		Arguments.Add(TEXT("2048"));
		Arguments.Add(TEXT("0"));
		Arguments.Add(TEXT("-fvk-s-shift"));
		Arguments.Add(TEXT("3072"));
		Arguments.Add(TEXT("0"));
		Arguments.Add(TEXT("-fvk-use-dx-layout"));
		Arguments.Add(TEXT("-fvk-use-scalar-layout"));
		Arguments.Add(TEXT("-fspv-entrypoint-name=main_00000000_00000000"));
		Arguments.Add(TEXT("-fspv-reflect"));
	}

	DxcBuffer SourceBuffer;
	SourceBuffer.Ptr = BlobSource->GetBufferPointer();
	SourceBuffer.Size = BlobSource->GetBufferSize();
	SourceBuffer.Encoding = 0;

	IDxcResult* CompileResult;
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

	if (RHIInterfaceType == ERHIInterfaceType::D3D12)
	{

#if PLATFORM_WINDOWS
		IDxcBlob* Reflection;
		HR = CompileResult->GetOutput(DXC_OUT_REFLECTION, __uuidof(IDxcBlob), reinterpret_cast<void**>(&Reflection), nullptr);
		if (!SUCCEEDED(HR))
		{
			ErrorMessages = "Unable to get reflection result";
			CompiledBlob->Release();
			CompileResult->Release();
			return false;
		}

		ID3D12ShaderReflection* ShaderReflection;

		DxcBuffer ReflectionBuffer;
		ReflectionBuffer.Ptr = Reflection->GetBufferPointer();
		ReflectionBuffer.Size = Reflection->GetBufferSize();
		ReflectionBuffer.Encoding = 0;
		HR = DXC::Utils->CreateReflection(&ReflectionBuffer, __uuidof(ID3D12ShaderReflection), reinterpret_cast<void**>(&ShaderReflection));
		if (!SUCCEEDED(HR))
		{
			ErrorMessages = "Unable to create reflection";
			Reflection->Release();
			CompiledBlob->Release();
			CompileResult->Release();
			return false;
		}

		D3D12_SHADER_DESC ShaderDesc;
		ShaderReflection->GetDesc(&ShaderDesc);

		TMap<uint32, D3D12_SHADER_INPUT_BIND_DESC> CBVMapping;
		TMap<uint32, D3D12_SHADER_INPUT_BIND_DESC> SRVMapping;
		TMap<uint32, D3D12_SHADER_INPUT_BIND_DESC> UAVMapping;
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

			switch (BindDesc.Type)
			{
			case D3D_SIT_CBUFFER:
				CBVMapping.Add(BindDesc.BindPoint, BindDesc);
				break;
			case D3D_SIT_TBUFFER:
			case D3D_SIT_TEXTURE:
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_BYTEADDRESS:
				SRVMapping.Add(BindDesc.BindPoint, BindDesc);
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWBYTEADDRESS:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_CONSUME_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			case D3D_SIT_UAV_FEEDBACKTEXTURE:
				UAVMapping.Add(BindDesc.BindPoint, BindDesc);
				break;
			default:
				ErrorMessages = FString::Printf(TEXT("Unsupported resource type for %s"), UTF8_TO_TCHAR(BindDesc.Name));
				ShaderReflection->Release();
				Reflection->Release();
				CompiledBlob->Release();
				CompileResult->Release();
				return false;
			}

		}

		ShaderReflection->Release();
		Reflection->Release();

		TArray<uint32> CBVKeys;
		CBVMapping.GetKeys(CBVKeys);
		CBVKeys.Sort();

		for (uint32 CBVIndex : CBVKeys)
		{
			D3D12_SHADER_INPUT_BIND_DESC& BindDesc = CBVMapping[CBVIndex];

			FCompushadyShaderResourceBinding ResourceBinding;
			ResourceBinding.SlotIndex = BindDesc.BindPoint;
			ResourceBinding.Name = BindDesc.Name;
			ResourceBinding.Type = ECompushadySharedResourceType::Buffer;

			ShaderResourceBindings.CBVs.Add(ResourceBinding);
		}

		TArray<uint32> SRVKeys;
		SRVMapping.GetKeys(SRVKeys);
		SRVKeys.Sort();

		for (uint32 SRVIndex : SRVKeys)
		{
			D3D12_SHADER_INPUT_BIND_DESC& BindDesc = SRVMapping[SRVIndex];

			FCompushadyShaderResourceBinding ResourceBinding;
			ResourceBinding.SlotIndex = BindDesc.BindPoint;
			ResourceBinding.Name = BindDesc.Name;
			ResourceBinding.Type = ECompushadySharedResourceType::Texture;

			ShaderResourceBindings.SRVs.Add(ResourceBinding);
		}

		TArray<uint32> UAVKeys;
		UAVMapping.GetKeys(UAVKeys);
		UAVKeys.Sort();

		for (uint32 UAVIndex : UAVKeys)
		{
			D3D12_SHADER_INPUT_BIND_DESC& BindDesc = UAVMapping[UAVIndex];

			FCompushadyShaderResourceBinding ResourceBinding;
			ResourceBinding.SlotIndex = BindDesc.BindPoint;
			ResourceBinding.Name = BindDesc.Name;
			ResourceBinding.Type = ECompushadySharedResourceType::Texture;

			ShaderResourceBindings.UAVs.Add(ResourceBinding);
		}
#endif
	}

	CompileResult->Release();

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

	ByteCode.Append(reinterpret_cast<const uint8*>(CompiledBlob->GetBufferPointer()), CompiledBlob->GetBufferSize());

	CompiledBlob->Release();

	return true;
}


void FCompushadyModule::StartupModule()
{
}

void FCompushadyModule::ShutdownModule()
{
	Compushady::DXC::Teardown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCompushadyModule, Compushady)