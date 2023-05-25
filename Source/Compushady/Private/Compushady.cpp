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

THIRD_PARTY_INCLUDES_START
#if PLATFORM_LINUX || PLATFORM_ANDROID
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

#include "Serialization/ArrayWriter.h"

#include "vulkan.h"
#include "VulkanCommon.h"
#include "VulkanShaderResources.h"

#if PLATFORM_WINDOWS
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if WITH_EDITOR
#include "CompushadyShader.h"
#include "CompushadyHLSLSyntaxHighlighter.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailCustomization.h"
#include "PropertyEditorModule.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Text/SMultiLineEditableText.h"
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
#elif PLATFORM_LINUX || PLATFORM_ANDROID
				LibHandle = FPlatformProcess::GetDllHandle(TEXT("libdxcompiler.so"));
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
		Arguments.Add(L"-fvk-t-shift");
		Arguments.Add(L"1024");
		Arguments.Add(L"0");
		Arguments.Add(L"-fvk-u-shift");
		Arguments.Add(L"2048");
		Arguments.Add(L"0");
		Arguments.Add(L"-fvk-s-shift");
		Arguments.Add(L"3072");
		Arguments.Add(L"0");
		Arguments.Add(L"-fvk-use-dx-layout");
		Arguments.Add(L"-fvk-use-scalar-layout");
		Arguments.Add(L"-fspv-entrypoint-name=main_00000000_00000000");
		Arguments.Add(L"-fspv-reflect");
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


	TMap<uint32, FCompushadyShaderResourceBinding> CBVMapping;
	TMap<uint32, FCompushadyShaderResourceBinding> SRVMapping;
	TMap<uint32, FCompushadyShaderResourceBinding> UAVMapping;

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
				ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
				CBVMapping.Add(BindDesc.BindPoint, ResourceBinding);
				break;
			case D3D_SIT_TEXTURE:
				ResourceBinding.Type = ECompushadySharedResourceType::Texture;
				SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
				break;
			case D3D_SIT_TBUFFER:
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_BYTEADDRESS:
				ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
				SRVMapping.Add(BindDesc.BindPoint, ResourceBinding);
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_FEEDBACKTEXTURE:
				ResourceBinding.Type = ECompushadySharedResourceType::Texture;
				UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
				break;
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWBYTEADDRESS:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_CONSUME_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
				ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
				UAVMapping.Add(BindDesc.BindPoint, ResourceBinding);
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

	if (RHIInterfaceType == ERHIInterfaceType::Vulkan)
	{
		FVulkanShaderHeader VulkanShaderHeader;
		VulkanShaderHeader.SpirvCRC = FCrc::MemCrc32(ByteCode.GetData(), ByteCode.Num());

		// the entry point is fixed to "main_00000000_00000000"
		// update it to pass the size/crc check
		ANSICHAR SpirVEntryPoint[24];
		FCStringAnsi::Snprintf(SpirVEntryPoint, 24, "main_%0.8x_%0.8x", ByteCode.Num(), VulkanShaderHeader.SpirvCRC);

		// SPIR-V is generally managed as an array of 32bit words
		TArrayView<uint32> SpirV = TArrayView<uint32>((uint32*)ByteCode.GetData(), ByteCode.Num() / sizeof(uint32));

		// skip the first 4 words
		int32 Offset = 5;

		struct FCompushadySpirVDecoration
		{
			uint32 Binding;
			uint32 BindingIndexOffset;
			uint32 DescriptorSetOffset;
			FString Name;
			FString ReflectionType;
		};

		// this is a map between SPIR-V ids bindings and reflection types
		TMap<uint32, FCompushadySpirVDecoration> Bindings;

		while (Offset < SpirV.Num())
		{
			uint32 Word = SpirV[Offset];
			uint16 Opcode = Word & 0xFFFF;
			uint16 Size = Word >> 16;
			if (Size == 0)
			{
				break;
			}

			// get the bindings/descriptor sets
			if (Opcode == 71 && (Offset + Size < SpirV.Num())) // OpDecorate(71) + id + Binding
			{
				if (Size > 3)
				{
					if (SpirV[Offset + 2] == 33) // Binding
					{
						FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 1]);
						Decoration.Binding = SpirV[Offset + 3];
						Decoration.BindingIndexOffset = Offset + 3;
					}
					else if (SpirV[Offset + 2] == 34) // DescriptorSet
					{
						FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 1]);
						Decoration.DescriptorSetOffset = Offset + 3;
					}
				}
			}
			// get the name
			else if (Opcode == 5 && (Offset + Size < SpirV.Num())) // OpName(5) + id + String
			{
				if (Size > 2)
				{
					const char* Name = reinterpret_cast<char*>(&SpirV[Offset + 2]);
					FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 1]);
					Decoration.Name = UTF8_TO_TCHAR(Name);
				}
			}
			// get the reflection friendly type
			else if (Opcode == 5632 && (Offset + Size < SpirV.Num())) // OpDecorateString(5632) + id + Decoration(UserTypeGOOGLE/5636) + String
			{
				if (Size > 3 && SpirV[Offset + 2] == 5636)
				{
					const char* DecorationString = reinterpret_cast<char*>(&SpirV[Offset + 3]);
					FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 1]);
					Decoration.ReflectionType = UTF8_TO_TCHAR(DecorationString);
				}
			}
			// patch the EntryPoint Name
			else if (Opcode == 15 && (Offset + Size < SpirV.Num())) // OpEntryPoint(15) + ExecutionModel + id + Name
			{
				if (Size > 8 && SpirV[Offset + 1] == 5) // ExecutionModel == GLCompute
				{
					uint32* EntryPointPtr = reinterpret_cast<uint32*>(SpirVEntryPoint);
					for (int32 Index = 0; Index < 6; Index++)
					{
						SpirV[Offset + 3 + Index] = EntryPointPtr[Index];
					}
				}
			}
			Offset += Size;
		}

		int32 UniformBufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::UniformBuffer);
		int32 ImageType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::Image);
		int32 BufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::UniformTexelBuffer);
		int32 StorageImageType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::StorageImage);
		int32 StorageBufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::StorageTexelBuffer);
		int32 StorageStructuredBufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::StorageBuffer);

		for (const TPair<uint32, FCompushadySpirVDecoration>& Pair : Bindings)
		{
			// skip empty offsets (they are not resources!)
			if (Pair.Value.BindingIndexOffset == 0)
			{
				continue;
			}

			FCompushadyShaderResourceBinding ResourceBinding;
			ResourceBinding.Name = Pair.Value.Name;

			FVulkanShaderHeader::FSpirvInfo SpirvInfo;
			SpirvInfo.BindingIndexOffset = Pair.Value.BindingIndexOffset;
			SpirvInfo.DescriptorSetOffset = Pair.Value.DescriptorSetOffset;

			if (Pair.Value.Binding < 1024) // CBV
			{
				FVulkanShaderHeader::FUniformBufferInfo UniformBufferInfo = {};
				UniformBufferInfo.ConstantDataOriginalBindingIndex = Pair.Value.Binding;

				ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
				ResourceBinding.BindingIndex = Pair.Value.Binding;
				ResourceBinding.SlotIndex = VulkanShaderHeader.UniformBuffers.Add(UniformBufferInfo);
				VulkanShaderHeader.UniformBufferSpirvInfos.Add(SpirvInfo);

				CBVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);

			}
			else if (Pair.Value.Binding < 2048) // SRV
			{
				FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
				GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
				GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;

				if (Pair.Value.ReflectionType.Contains("texture"))
				{
					GlobalInfo.TypeIndex = ImageType;
					ResourceBinding.Type = ECompushadySharedResourceType::Texture;
				}
				else
				{
					if (Pair.Value.ReflectionType.Contains("structured"))
					{
						GlobalInfo.TypeIndex = StorageStructuredBufferType;
					}
					else
					{
						GlobalInfo.TypeIndex = BufferType;
					}
					ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
				}

				ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
				ResourceBinding.BindingIndex = Pair.Value.Binding - 1024;
				VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

				SRVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
			}
			else if (Pair.Value.Binding < 3072) // UAV
			{
				FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
				GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
				GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;

				if (Pair.Value.ReflectionType.Contains("texture"))
				{
					GlobalInfo.TypeIndex = StorageImageType;
					ResourceBinding.Type = ECompushadySharedResourceType::Texture;
				}
				else
				{
					if (Pair.Value.ReflectionType.Contains("structured"))
					{
						GlobalInfo.TypeIndex = StorageStructuredBufferType;
					}
					else
					{
						GlobalInfo.TypeIndex = StorageBufferType;
					}
					ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
				}

				ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
				ResourceBinding.BindingIndex = Pair.Value.Binding - 2048;
				VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

				UAVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
			}
			else
			{
				ErrorMessages = FString::Printf(TEXT("Invalid shader binding for %s: %u"), *ResourceBinding.Name, Pair.Value.Binding);
				return false;
			}
		}

		// if on Android, we need to remove reflection opcodes (by setting to OpNop)
#if PLATFORM_ANDROID
		Offset = 5;
		while (Offset < SpirV.Num())
		{
			uint32 Word = SpirV[Offset];
			uint16 Opcode = Word & 0xFFFF;
			uint16 Size = Word >> 16;
			if (Size == 0)
			{
				break;
			}

			// strip OpDecorateStringGOOGLE
			if (Opcode == 5632 && (Offset + Size < SpirV.Num()) && Size > 2) // OpDecorateStringGOOGLE(5632) + id + Decoration(UserTypeGOOGLE/5636|HlslSemanticGOOGLE/5635) + String
			{
				if (SpirV[Offset + 2] == 5636 || SpirV[Offset + 2] == 5635)
				{
					for (int32 Index = 0; Index < Size; Index++)
					{
						SpirV[Offset + Index] = 0x00010000;
					}
				}
			}

			// strip OpMemberDecorateStringGOOGLE
			else if (Opcode == 5633 && (Offset + Size < SpirV.Num()) && Size > 3) // OpMemberDecorateStringGOOGLE(5632) + id + member + Decoration(UserTypeGOOGLE/5636|HlslSemanticGOOGLE/5635) + String
			{
				if (SpirV[Offset + 3] == 5636 || SpirV[Offset + 3] == 5635)
				{
					for (int32 Index = 0; Index < Size; Index++)
					{
						SpirV[Offset + Index] = 0x00010000;
					}
				}
			}

			// strip reflection extensions
			else if (Opcode == 10 && (Offset + Size < SpirV.Num()) && Size > 1)
			{
				const char* ExtensionName = reinterpret_cast<char*>(&SpirV[Offset + 1]);
				FString ExtensionNameString = UTF8_TO_TCHAR(ExtensionName);
				if (ExtensionNameString == "SPV_GOOGLE_hlsl_functionality1" || ExtensionNameString == "SPV_GOOGLE_user_type")
				{
					for (int32 Index = 0; Index < Size; Index++)
					{
						SpirV[Offset + Index] = 0x00010000;
					}
				}
	}

			Offset += Size;
}
#endif

		FArrayWriter Writer;

		Writer << VulkanShaderHeader;

		int32 SpirvSize = ByteCode.Num();

		// a bit annoying, but we need to manage the const here
		Writer << SpirvSize;
		Writer.Serialize(const_cast<void*>(reinterpret_cast<const void*>(ByteCode.GetData())), ByteCode.Num());

		int32 MinusOne = -1;

		Writer << MinusOne;

		ByteCode = Writer;
	}

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


	return true;
}

#if WITH_EDITOR
class FCompushadyShaderCustomization : public IDetailCustomization
{
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		TArray<TWeakObjectPtr<UObject>> Objects;
		DetailBuilder.GetObjectsBeingCustomized(Objects);

		if (Objects.Num() != 1)
		{
			return;
		}

		TWeakObjectPtr<UCompushadyShader> CompushadyShader = Cast<UCompushadyShader>(Objects[0].Get());

		TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UCompushadyShader, Code));
		DetailBuilder.HideProperty(PropertyHandle);


		IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory("Code");


		CategoryBuilder.AddCustomRow(FText::FromString("Code")).WholeRowContent()[

			SNew(SVerticalBox)
			+SVerticalBox::Slot().MaxHeight(800)
			[
			SNew(SMultiLineEditableTextBox)
				.AutoWrapText(false)
				.Margin(0.0f)
				.Marshaller(FCompushadyHLSLSyntaxHighlighter::Create())
				.Text(FText::FromString(CompushadyShader->Code))
				.OnTextChanged_Lambda([CompushadyShader](const FText& InCode)
					{
						CompushadyShader->Code = InCode.ToString();
			CompushadyShader->MarkPackageDirty();
					})
			]
			+SVerticalBox::Slot().FillHeight(0.1)
			[
				SNew(STextBlock).Text(FText::FromString("Test"))
			]

		];
	}

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShared<FCompushadyShaderCustomization>();
	}

};
#endif

void FCompushadyModule::StartupModule()
{
#if WITH_EDITOR
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyModule.RegisterCustomClassLayout(TEXT("CompushadyShader"), FOnGetDetailCustomizationInstance::CreateStatic(&FCompushadyShaderCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
#endif
}

void FCompushadyModule::ShutdownModule()
{
	Compushady::DXC::Teardown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCompushadyModule, Compushady)
