// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

#include "Serialization/ArrayWriter.h"

#if PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_ANDROID
#include "vulkan.h"
#include "VulkanCommon.h"
#include "VulkanShaderResources.h"


bool Compushady::FixupSPIRV(TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	TMap<uint32, FCompushadyShaderResourceBinding> CBVMapping;
	TMap<uint32, FCompushadyShaderResourceBinding> SRVMapping;
	TMap<uint32, FCompushadyShaderResourceBinding> UAVMapping;

	// SPIR-V is generally managed as an array of 32bit words
	TArrayView<uint32> SpirV = TArrayView<uint32>((uint32*)ByteCode.GetData(), ByteCode.Num() / sizeof(uint32));

	// skip the first 4 words
	int32 Offset = 5;

	int32 EntryPointCharsCounter = 0;
	int32 EntryPointOffset = 0;
	// First of all, let's check for the entry point (it may have been generated manually, we expect 22 + 1 chars (6 words))
	while (Offset < SpirV.Num())
	{
		uint32 Word = SpirV[Offset];
		uint16 Opcode = Word & 0xFFFF;
		uint16 Size = Word >> 16;
		if (Size == 0)
		{
			break;
		}

		if (Opcode == 15 && (Offset + Size < SpirV.Num()) && Size > 3) // OpEntryPoint(15) + ExecutionModel(GLCompute/*) + id + Name + ...)
		{
			EntryPointOffset = Offset;
			bool bFoundNull = false;
			for (int32 Index = 0; Index < Size; Index++)
			{
				uint8* CharPtr = reinterpret_cast<uint8*>(&SpirV[Offset + 3 + Index]);
				for (int32 CharIndex = 0; CharIndex < 4; CharIndex++)
				{
					EntryPointCharsCounter++;
					if (CharPtr[CharIndex] == 0)
					{
						bFoundNull = true;
						break;
					}
				}
				if (bFoundNull)
				{
					break;
				}
			}
			break;
		}
		Offset += Size;
	}

	if (EntryPointOffset == 0)
	{
		ErrorMessages = "Unable to find SPIRV EntryPoint";
		return false;
	}

	if (EntryPointCharsCounter < 23)
	{
		int32 WordsAvailable = (EntryPointCharsCounter / 4) + ((EntryPointCharsCounter % 4 != 0) ? 1 : 0);
		ByteCode.InsertZeroed((EntryPointOffset + 3) * 4, (6 - WordsAvailable) * 4);
		// remap the array view as the internal pointer could have been changed
		SpirV = TArrayView<uint32>((uint32*)ByteCode.GetData(), ByteCode.Num() / sizeof(uint32));
		// fix the opcode + size
		uint32 Word = SpirV[Offset];
		uint16 Opcode = Word & 0xFFFF;
		uint16 Size = Word >> 16;
		Size += 6 - WordsAvailable;
		SpirV[Offset] = Opcode | Size << 16;
	}
	// we need to reduce the blob size
	else if (EntryPointCharsCounter > 23)
	{
		// nop
	}

	FVulkanShaderHeader VulkanShaderHeader;
	VulkanShaderHeader.SpirvCRC = FCrc::MemCrc32(ByteCode.GetData(), ByteCode.Num());
	VulkanShaderHeader.InOutMask = 0xffffffff;

	// the entry point is fixed to "main_00000000_00000000"
	// update it to pass the size/crc check
	ANSICHAR SpirVEntryPoint[24];
	FCStringAnsi::Snprintf(SpirVEntryPoint, 24, "main_%0.8x_%0.8x", ByteCode.Num(), VulkanShaderHeader.SpirvCRC);

	Offset = 5;

	struct FCompushadySpirVDecoration
	{
		uint32 Binding;
		uint32 BindingIndexOffset;
		uint32 DescriptorSetOffset;
		uint32 TypeId;
		FString Name;
		FString ReflectionType;
		bool bHasBinding;

		FCompushadySpirVDecoration()
		{
			Binding = 0;
			BindingIndexOffset = 0;
			DescriptorSetOffset = 0;
			TypeId = 0;
			bHasBinding = false;
		}
	};

	// this is a map between SPIR-V ids bindings and reflection types
	TMap<uint32, FCompushadySpirVDecoration> Bindings;

	// store the pointer types (as a fallback when reflection is not available)
	TMap<uint32, uint32> SpirVPointers;
	// store the relevant types (Struct and Images)
	TMap<uint32, uint32> SpirVStructs;
	TMap<uint32, TPair<uint32, uint32>> SpirVImages;
	TMap<uint32, uint32> SpirVSampledImages;
	// track Block decorations (for recognizing CBVs)
	TSet<uint32> SpirVBlocks;

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
					Decoration.bHasBinding = true;
				}
				else if (SpirV[Offset + 2] == 34) // DescriptorSet
				{
					FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 1]);
					Decoration.DescriptorSetOffset = Offset + 3;
				}
			}
			else if (Size > 2 && SpirV[Offset + 2] == 2) // Block
			{
				SpirVBlocks.Add(SpirV[Offset + 1]);
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
		else if (Opcode == 15 && (Offset + Size < SpirV.Num()) && Size > 8) // OpEntryPoint(15) + ExecutionModel + id + Name + ...
		{
			uint32* EntryPointPtr = reinterpret_cast<uint32*>(SpirVEntryPoint);
			for (int32 Index = 0; Index < 6; Index++)
			{
				SpirV[Offset + 3 + Index] = EntryPointPtr[Index];
			}
		}
		else if (Opcode == 59 && (Offset + Size < SpirV.Num()) && Size > 3) // OpVariable + id_type + id + StorageClass
		{
			FCompushadySpirVDecoration& Decoration = Bindings.FindOrAdd(SpirV[Offset + 2]);
			Decoration.TypeId = SpirV[Offset + 1];
		}
		else if (Opcode == 32 && (Offset + Size < SpirV.Num()) && Size > 3) // OpTypePointer + id + StorageClass + id_type
		{
			SpirVPointers.Add(SpirV[Offset + 1], SpirV[Offset + 3]);
		}
		else if (Opcode == 25 && (Offset + Size < SpirV.Num()) && Size > 8) // OpTypeImage + id + ... Dim + Sampled
		{
			SpirVImages.Add(SpirV[Offset + 1], TPair<uint32, uint32>(SpirV[Offset + 3], SpirV[Offset + 7]));
		}
		else if (Opcode == 27 && (Offset + Size < SpirV.Num()) && Size > 2) // OpTypeSampledImage + id + id_type
		{
			SpirVSampledImages.Add(SpirV[Offset + 1], SpirV[Offset + 2]);
		}
		else if (Opcode == 30 && (Offset + Size < SpirV.Num()) && Size > 1) // OpTypeStruct + id + ...
		{
			SpirVStructs.Add(SpirV[Offset + 1], Opcode);
		}
		else if (Opcode == 16 && (Offset + Size < SpirV.Num()) && Size > 5 && SpirV[Offset + 2] == 17) // OpExecutionMode + id + LocalSize(17) + X + Y + Z ...
		{
			ThreadGroupSize.X = SpirV[Offset + 3];
			ThreadGroupSize.Y = SpirV[Offset + 4];
			ThreadGroupSize.Z = SpirV[Offset + 5];
		}
		Offset += Size;
	}

	// fixup SampledImages
	for (TPair<uint32, uint32>& Pair : SpirVPointers)
	{
		if (SpirVSampledImages.Contains(Pair.Value))
		{
			Pair.Value = SpirVSampledImages[Pair.Value];
		}
	}

	int32 UniformBufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::UniformBuffer);
	int32 ImageType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::Image);
	int32 BufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::UniformTexelBuffer);
	int32 StorageImageType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::StorageImage);
	int32 StorageBufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::StorageTexelBuffer);
	int32 StorageStructuredBufferType = VulkanShaderHeader.GlobalDescriptorTypes.Add(EVulkanBindingType::StorageBuffer);

	for (const TPair<uint32, FCompushadySpirVDecoration>& Pair : Bindings)
	{
		// skip unbound decorations
		if (!Pair.Value.bHasBinding)
		{
			continue;
		}

		FCompushadyShaderResourceBinding ResourceBinding;
		ResourceBinding.Name = Pair.Value.Name;

		FVulkanShaderHeader::FSpirvInfo SpirvInfo;
		SpirvInfo.BindingIndexOffset = Pair.Value.BindingIndexOffset;
		SpirvInfo.DescriptorSetOffset = Pair.Value.DescriptorSetOffset;

		if (Pair.Value.ReflectionType.IsEmpty() && ResourceBinding.Name != "$Globals")
		{
			// this code path tries to do its best to rebuild the pipeline without reflection
			if (SpirVPointers.Contains(Pair.Value.TypeId))
			{
				uint32 TypeId = SpirVPointers[Pair.Value.TypeId];
				// identify CBVs
				if (SpirVBlocks.Contains(TypeId))
				{
					FVulkanShaderHeader::FUniformBufferInfo UniformBufferInfo = {};
					UniformBufferInfo.ConstantDataOriginalBindingIndex = Pair.Value.Binding;

					ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
					ResourceBinding.BindingIndex = Pair.Value.Binding;
					ResourceBinding.SlotIndex = VulkanShaderHeader.UniformBuffers.Add(UniformBufferInfo);
					VulkanShaderHeader.UniformBufferSpirvInfos.Add(SpirvInfo);

					CBVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
					continue;
				}
				else
				{
					if (SpirVImages.Contains(TypeId))
					{
						if (SpirVImages[TypeId].Key < 5) // Texture?
						{
							if (SpirVImages[TypeId].Value < 2) // SRV?
							{
								FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
								GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
								GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
								GlobalInfo.TypeIndex = ImageType;
								ResourceBinding.Type = ECompushadySharedResourceType::Texture;
								ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
								ResourceBinding.BindingIndex = Pair.Value.Binding;
								VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

								SRVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
								continue;
							}
							else // UAV
							{
								FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
								GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
								GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
								GlobalInfo.TypeIndex = StorageImageType;
								ResourceBinding.Type = ECompushadySharedResourceType::Texture;
								ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
								ResourceBinding.BindingIndex = Pair.Value.Binding;
								VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

								UAVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
								continue;
							}
						}
						else // buffer
						{
							if (SpirVImages[TypeId].Value < 2) // SRV?
							{
								FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
								GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
								GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
								GlobalInfo.TypeIndex = BufferType;
								ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
								ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
								ResourceBinding.BindingIndex = Pair.Value.Binding;
								VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

								SRVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
								continue;
							}
							else // UAV
							{
								FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
								GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
								GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
								GlobalInfo.TypeIndex = StorageBufferType;
								ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
								ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
								ResourceBinding.BindingIndex = Pair.Value.Binding;
								VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

								UAVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
								continue;
							}
						}
					}
				}
			}
			ErrorMessages = FString::Printf(TEXT("Reflection data unavailable for %s (binding:%u)"), *ResourceBinding.Name, Pair.Value.Binding);
			return false;
		}
		else if (Pair.Value.ReflectionType == "cbuffer" || ResourceBinding.Name == "$Globals")
		{
			FVulkanShaderHeader::FUniformBufferInfo UniformBufferInfo = {};
			UniformBufferInfo.ConstantDataOriginalBindingIndex = Pair.Value.Binding;

			ResourceBinding.Type = ECompushadySharedResourceType::UniformBuffer;
			ResourceBinding.BindingIndex = Pair.Value.Binding;
			ResourceBinding.SlotIndex = VulkanShaderHeader.UniformBuffers.Add(UniformBufferInfo);
			VulkanShaderHeader.UniformBufferSpirvInfos.Add(SpirvInfo);

			CBVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
		}
		else if (Pair.Value.ReflectionType.StartsWith("buffer:"))
		{
			FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
			GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
			GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
			GlobalInfo.TypeIndex = BufferType;
			ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
			ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
			ResourceBinding.BindingIndex = Pair.Value.Binding;
			VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

			SRVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
		}
		else if (Pair.Value.ReflectionType.StartsWith("rwbuffer:"))
		{
			FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
			GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
			GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
			GlobalInfo.TypeIndex = StorageBufferType;
			ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
			ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
			ResourceBinding.BindingIndex = Pair.Value.Binding;
			VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

			UAVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
		}
		else if (Pair.Value.ReflectionType == "byteaddressbuffer" || Pair.Value.ReflectionType.StartsWith("structuredbuffer:"))
		{
			FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
			GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
			GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
			GlobalInfo.TypeIndex = StorageStructuredBufferType;
			ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
			ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
			ResourceBinding.BindingIndex = Pair.Value.Binding;
			VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

			SRVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
		}
		else if (Pair.Value.ReflectionType.StartsWith("rwstructuredbuffer:") ||
			Pair.Value.ReflectionType == "rwbyteaddressbuffer"
			/* || Pair.Value.ReflectionType.StartsWith("appendstructuredbuffer:") */)
		{
			FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
			GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
			GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
			GlobalInfo.TypeIndex = StorageStructuredBufferType;
			ResourceBinding.Type = ECompushadySharedResourceType::Buffer;
			ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
			ResourceBinding.BindingIndex = Pair.Value.Binding;
			VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

			UAVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
		}
		else if (Pair.Value.ReflectionType.StartsWith("texture"))
		{
			FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
			GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
			GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
			GlobalInfo.TypeIndex = ImageType;
			ResourceBinding.Type = ECompushadySharedResourceType::Texture;
			ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
			ResourceBinding.BindingIndex = Pair.Value.Binding;
			VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

			SRVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
		}
		else if (Pair.Value.ReflectionType.StartsWith("rwtexture"))
		{
			FVulkanShaderHeader::FGlobalInfo GlobalInfo = {};
			GlobalInfo.OriginalBindingIndex = Pair.Value.Binding;
			GlobalInfo.CombinedSamplerStateAliasIndex = UINT16_MAX;
			GlobalInfo.TypeIndex = StorageImageType;
			ResourceBinding.Type = ECompushadySharedResourceType::Texture;
			ResourceBinding.SlotIndex = VulkanShaderHeader.Globals.Add(GlobalInfo);
			ResourceBinding.BindingIndex = Pair.Value.Binding;
			VulkanShaderHeader.GlobalSpirvInfos.Add(SpirvInfo);

			UAVMapping.Add(ResourceBinding.BindingIndex, ResourceBinding);
		}
		else
		{
			ErrorMessages = FString::Printf(TEXT("Unsupported shader resource type \"%s\" for %s (binding: %u)"), *Pair.Value.ReflectionType, *ResourceBinding.Name, Pair.Value.Binding);
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

#if COMPUSHADY_UE_VERSION >= 53
	FShaderResourceTable ShaderResourceTable;
	Writer << ShaderResourceTable;
#endif

	int32 SpirvSize = ByteCode.Num();

	Writer << SpirvSize;

	// a bit annoying, but we need to manage the const here
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
#else
bool Compushady::FixupSPIRV(TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FIntVector& ThreadGroupSize, FString& ErrorMessages)
{
	return false;
}
#endif
