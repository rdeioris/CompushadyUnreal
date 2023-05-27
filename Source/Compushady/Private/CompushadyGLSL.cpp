// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

namespace Compushady
{
	namespace KHR
	{
		static void* LibHandle = nullptr;
		static void* (*GLSLToSpirV)(const char* Source, const int SourceLen, void* (*Allocator)(size_t), size_t* SpirVSize, char** Errors) = nullptr;

		static bool Setup()
		{
			if (!LibHandle)
			{
#if PLATFORM_WINDOWS
#if WITH_EDITOR
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectPluginsDir() / TEXT("Compushady/Binaries/Win64/libcompushady_khr.dll")));
#else
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectDir() / TEXT("Binaries/Win64/libcompushady_khr.dll")));
#endif
#elif PLATFORM_LINUX || PLATFORM_ANDROID
				LibHandle = FPlatformProcess::GetDllHandle(TEXT("libcompushady_khr.so"));
#endif
				if (!LibHandle)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to load libcompushady_khr shared library"));
					return false;
				}
			}

			if (!GLSLToSpirV)
			{
				GLSLToSpirV = reinterpret_cast<void* (*)(const char*, const int, void* (*)(SIZE_T), SIZE_T*, char**)>(FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_glslang_compile")));

				if (!GLSLToSpirV)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_glslang_compile symbol in libcompushady_khr"));
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
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T SpvSize;
	char* Errors = nullptr;
	void* Data = KHR::GLSLToSpirV((const char*)ShaderCode.GetData(), ShaderCode.Num(), KHR::Malloc, &SpvSize, &Errors);
	if (!Data)
	{
		if (Errors)
		{
			ErrorMessages = UTF8_TO_TCHAR(Errors);
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to compile GLSL sources";
		}
		return false;
	}

	ByteCode.Append((uint8*)Data, SpvSize);
	FMemory::Free(Data);

	return FixupSPIRV(ByteCode, ShaderResourceBindings, ErrorMessages);
}