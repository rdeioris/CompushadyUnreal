// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

namespace Compushady
{
	namespace EasyGlslang
	{
		static void* LibHandle = nullptr;
		static void* (*GLSLToSpirV)(const char* source, void* (*allocator)(size_t), size_t* spirv_size) = nullptr;

		static bool Setup()
		{
			if (!LibHandle)
			{
#if PLATFORM_WINDOWS
#if WITH_EDITOR
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectPluginsDir() / TEXT("Compushady/Binaries/Win64/easy_glslang.dll")));
#else
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectDir() / TEXT("Binaries/Win64/easy_glslang.dll")));
#endif
#elif PLATFORM_LINUX || PLATFORM_ANDROID
				LibHandle = FPlatformProcess::GetDllHandle(TEXT("easy_glslang.so"));
#endif
				if (!LibHandle)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to load easy_glslang shared library"));
					return false;
				}
			}

			if (!GLSLToSpirV)
			{
				GLSLToSpirV = reinterpret_cast<void* (*)(const char* source, void* (*allocator)(SIZE_T), SIZE_T * spirv_size)>(FPlatformProcess::GetDllExport(LibHandle, TEXT("glslang_compile")));

				if (!GLSLToSpirV)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find glslang_compile symbol in easy_glslang"));
					return false;
				}
			}

			return true;
		}

		static void* Malloc(SIZE_T Size)
		{
			return FMemory::Malloc(Size);
		}
	}
}

bool Compushady::CompileGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FCompushadyShaderResourceBindings& ShaderResourceBindings, FString& ErrorMessages)
{
	if (!EasyGlslang::Setup())
	{
		return false;
	}

	SIZE_T SpvSize;
	void* Data = EasyGlslang::GLSLToSpirV((const char*)ShaderCode.GetData(), EasyGlslang::Malloc, &SpvSize);
	UE_LOG(LogTemp, Error, TEXT("Data: %p %llu %u"), Data, SpvSize, ((uint32*)Data)[0]);
	if (Data)
	{
		ByteCode.Append((uint8*)Data, SpvSize);
		FMemory::Free(Data);

		return FixupSPIRV(ByteCode, ShaderResourceBindings, ErrorMessages);
	}
	return false;
}