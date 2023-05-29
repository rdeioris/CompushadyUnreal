// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

namespace Compushady
{
	namespace KHR
	{
		static void* LibHandle = nullptr;
		static void* (*GLSLToSpirV)(const char* Source, const int SourceLen, void* (*Allocator)(SIZE_T), SIZE_T* SpirVSize, char** Errors) = nullptr;
		static void* (*SpirVDisassemble)(const uint32* Binary, const SIZE_T WordCount, void* (*Allocator)(SIZE_T), SIZE_T* DisassembledSize, char** Errors) = nullptr;
		static void* (*SpirVToHLSL)(const uint32* Binary, const SIZE_T WordCount, void* (*Allocator)(SIZE_T), SIZE_T* OutputSize, char** Errors) = nullptr;
		static void* (*SpirVToGLSL)(const uint32* Binary, const SIZE_T WordCount, void* (*Allocator)(SIZE_T), SIZE_T* OutputSize, char** Errors) = nullptr;
		static void* (*SpirVToMSL)(const uint32* Binary, const SIZE_T WordCount, void* (*Allocator)(SIZE_T), SIZE_T* OutputSize, char** Errors) = nullptr;

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

			if (!SpirVDisassemble)
			{
				SpirVDisassemble = reinterpret_cast<void* (*)(const uint32*, const SIZE_T, void* (*)(SIZE_T), SIZE_T*, char**)>(FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_spirv_disassemble")));

				if (!SpirVDisassemble)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_spirv_disassemble symbol in libcompushady_khr"));
					return false;
				}
			}

			if (!SpirVToHLSL)
			{
				SpirVToHLSL = reinterpret_cast<void* (*)(const uint32*, const SIZE_T, void* (*)(SIZE_T), SIZE_T*, char**)>(FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_spirv_to_hlsl")));

				if (!SpirVToHLSL)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_spirv_to_hlsl symbol in libcompushady_khr"));
					return false;
				}
			}

			if (!SpirVToGLSL)
			{
				SpirVToGLSL = reinterpret_cast<void* (*)(const uint32*, const SIZE_T, void* (*)(SIZE_T), SIZE_T*, char**)>(FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_spirv_to_glsl")));

				if (!SpirVToGLSL)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_spirv_to_glsl symbol in libcompushady_khr"));
					return false;
				}
			}

			if (!SpirVToMSL)
			{
				SpirVToMSL = reinterpret_cast<void* (*)(const uint32*, const SIZE_T, void* (*)(SIZE_T), SIZE_T*, char**)>(FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_spirv_to_msl")));

				if (!SpirVToMSL)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_spirv_to_msl symbol in libcompushady_khr"));
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
	void* Data = KHR::GLSLToSpirV(reinterpret_cast<const char*>(ShaderCode.GetData()), ShaderCode.Num(), KHR::Malloc, &SpvSize, &Errors);
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

	ByteCode.Append(reinterpret_cast<uint8*>(Data), SpvSize);
	FMemory::Free(Data);

	return true;
}

bool Compushady::DisassembleSPIRV(const TArray<uint8>& ByteCode, FString& Disassembled, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T DisassembledSize;
	char* Errors = nullptr;
	void* Data = KHR::SpirVDisassemble(reinterpret_cast<const uint32*>(ByteCode.GetData()), ByteCode.Num() / 4, KHR::Malloc, &DisassembledSize, &Errors);
	if (!Data)
	{
		if (Errors)
		{
			ErrorMessages = UTF8_TO_TCHAR(Errors);
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to disassemble SPIRV";
		}
		return false;
	}

	Disassembled = UTF8_TO_TCHAR(Data);
	FMemory::Free(Data);

	return true;
}

bool Compushady::SPIRVToHLSL(const TArray<uint8>& ByteCode, FString& HLSL, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T OutputSize = 0;
	char* Errors = nullptr;
	void* Data = KHR::SpirVToHLSL(reinterpret_cast<const uint32*>(ByteCode.GetData()), ByteCode.Num() / 4, KHR::Malloc, &OutputSize, &Errors);
	if (!Data)
	{
		if (Errors)
		{
			ErrorMessages = UTF8_TO_TCHAR(Errors);
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to convert SPIRV to HLSL";
		}
		return false;
	}

	HLSL = UTF8_TO_TCHAR(Data);
	FMemory::Free(Data);

	return true;
}

bool Compushady::SPIRVToGLSL(const TArray<uint8>& ByteCode, FString& GLSL, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T OutputSize = 0;
	char* Errors = nullptr;
	void* Data = KHR::SpirVToGLSL(reinterpret_cast<const uint32*>(ByteCode.GetData()), ByteCode.Num() / 4, KHR::Malloc, &OutputSize, &Errors);
	if (!Data)
	{
		if (Errors)
		{
			ErrorMessages = UTF8_TO_TCHAR(Errors);
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to convert SPIRV to GLSL";
		}
		return false;
	}

	GLSL = UTF8_TO_TCHAR(Data);
	FMemory::Free(Data);

	return true;
}

bool Compushady::SPIRVToMSL(const TArray<uint8>& ByteCode, FString& MSL, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T OutputSize = 0;
	char* Errors = nullptr;
	void* Data = KHR::SpirVToMSL(reinterpret_cast<const uint32*>(ByteCode.GetData()), ByteCode.Num() / 4, KHR::Malloc, &OutputSize, &Errors);
	if (!Data)
	{
		if (Errors)
		{
			ErrorMessages = UTF8_TO_TCHAR(Errors);
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to convert SPIRV to MSL";
		}
		return false;
	}

	MSL = UTF8_TO_TCHAR(Data);
	FMemory::Free(Data);

	return true;
}