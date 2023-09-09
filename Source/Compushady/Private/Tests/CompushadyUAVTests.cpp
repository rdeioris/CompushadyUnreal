// Copyright 2023 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyUAVTest_Buffer, "Compushady.UAV.Buffer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyUAVTest_Buffer::RunTest(const FString& Parameters)
{
	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 32, EPixelFormat::PF_R32_UINT);

	UAV->MapWriteAndExecuteSync([](void* Data)
		{
			uint32* Ptr = reinterpret_cast<uint32*>(Data);
			for (int32 Index = 0; Index < 8; Index++)
			{
				Ptr[Index] = Index;
			}
		});


	TArray<uint32> Output;
	Output.AddUninitialized(8);

	UAV->MapReadAndExecuteSync([&Output](const void* Data)
		{
			const uint32* Ptr = reinterpret_cast<const uint32*>(Data);
			FMemory::Memcpy(Output.GetData(), Data, Output.Num() * sizeof(uint32));
		});


	TestEqual(TEXT("Output[0]"), Output[0], 0);
	TestEqual(TEXT("Output[1]"), Output[1], 1);
	TestEqual(TEXT("Output[2]"), Output[2], 2);
	TestEqual(TEXT("Output[3]"), Output[3], 3);
	TestEqual(TEXT("Output[4]"), Output[4], 4);
	TestEqual(TEXT("Output[5]"), Output[5], 5);
	TestEqual(TEXT("Output[6]"), Output[6], 6);
	TestEqual(TEXT("Output[7]"), Output[7], 7);

	return true;
}

#endif
