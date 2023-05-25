// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

#include "Serialization/ArrayWriter.h"

#include "vulkan.h"
#include "VulkanCommon.h"
#include "VulkanShaderResources.h"

bool Compushady::FixupSPIRV(TArray<uint8>& ByteCode, const FString& EntryPoint, const FString& TargetProfile, FCompushadyShaderResourceBindings& ShaderResourceBindings, FString& ErrorMessages)
{
	TMap<uint32, FCompushadyShaderResourceBinding> CBVMapping;
	TMap<uint32, FCompushadyShaderResourceBinding> SRVMapping;
	TMap<uint32, FCompushadyShaderResourceBinding> UAVMapping;

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

	return true;
}