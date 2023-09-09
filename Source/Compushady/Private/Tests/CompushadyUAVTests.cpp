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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyUAVTest_StructuredBuffer, "Compushady.UAV.StructuredBuffer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyUAVTest_StructuredBuffer::RunTest(const FString& Parameters)
{
	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVStructuredBuffer(TestName, 32, 32);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyUAVTest_MapTextureRead, "Compushady.UAV.MapTextureRead", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyUAVTest_MapTextureRead::RunTest(const FString& Parameters)
{
	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVTexture1D(TestName, 64, EPixelFormat::PF_R32_UINT);

	bool bSuccess = UAV->MapWriteAndExecuteSync([](void* Data)
		{
			// nop
		});


	TestFalse(TEXT("bSuccess"), bSuccess);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyUAVTest_MapTextureWrite, "Compushady.UAV.MapTextureWrite", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyUAVTest_MapTextureWrite::RunTest(const FString& Parameters)
{
	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVTexture1D(TestName, 64, EPixelFormat::PF_R32_UINT);

	bool bSuccess = UAV->MapReadAndExecuteSync([](const void* Data)
		{
			// nop
		});


	TestFalse(TEXT("bSuccess"), bSuccess);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyUAVTest_Texture2DArray, "Compushady.UAV.Texture2DArray", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyUAVTest_Texture2DArray::RunTest(const FString& Parameters)
{
	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVTexture2DArray(TestName, 8, 8, 4, EPixelFormat::PF_R32G32_UINT);

	TArray<uint64> Slices;
	for (int32 Index = 0; Index < 8 * 8 * 4; Index++)
	{
		uint64 Value0 = Index;
		uint64 Value1 = Index * 2;
		Slices.Add(Value0 | Value1 << 32);
	}

	bool bSuccess = UAV->UpdateTextureSync(reinterpret_cast<uint8*>(Slices.GetData()), 8 * 8 * 8, FCompushadyCopyInfo());

	TestTrue(TEXT("bSuccess"), bSuccess);

	return true;
}

#endif
