// Copyright 2024 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyGLSLTest_ThreadGroupSize, "Compushady.GLSL.ThreadGroupSize", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyGLSLTest_ThreadGroupSize::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString Code = "#version 450\nlayout(local_size_x = 3, local_size_y = 4, local_size_z = 5) in; void main() {}\n";

	UCompushadyCompute* Compute = UCompushadyFunctionLibrary::CreateCompushadyComputeFromGLSLString(Code, ErrorMessages, "main");

	TestEqual(TEXT("ThreadGroupSize.X"), Compute->GetThreadGroupSize().X, 3);
	TestEqual(TEXT("ThreadGroupSize.Y"), Compute->GetThreadGroupSize().Y, 4);
	TestEqual(TEXT("ThreadGroupSize.Z"), Compute->GetThreadGroupSize().Z, 5);

	return true;
}

#endif
