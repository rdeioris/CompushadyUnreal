// Copyright 2023 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyUAVTest_2DArray, "Compushady.UAV.2DArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyUAVTest_2DArray::RunTest(const FString& Parameters)
{
	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVTexture2DArray(TestName, 8, 8, 4, EPixelFormat::PF_R32_UINT);
	
	

	//TestFalse(TEXT("bSuccess"), bSuccess);

	return true;
}

#endif
