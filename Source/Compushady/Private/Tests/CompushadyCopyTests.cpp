// Copyright 2023-2024 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"

class FCompushadyWaitCopy : public IAutomationLatentCommand
{
public:
	FCompushadyWaitCopy(FAutomationTestBase* InTest, UCompushadyResource* InResource, TFunction<void()> InTestsFunction) : Test(InTest), Resource(InResource), TestsFunction(InTestsFunction)
	{

	}

	bool Update() override
	{
		if (!Resource->IsRunning())
		{
			TestsFunction();
		}
		return !Resource->IsRunning();
	}

private:
	FAutomationTestBase* Test;
	TStrongObjectPtr<UCompushadyResource> Resource;
	TFunction<void()> TestsFunction;
};


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCopyTest_Simple, "Compushady.Copy.Simple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCopyTest_Simple::RunTest(const FString& Parameters)
{
	UCompushadyUAV* Source = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 8, EPixelFormat::PF_R32_FLOAT);
	Source->ClearBufferWithFloatSync(17);

	UCompushadyUAV* Destination = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName + "_", 8, EPixelFormat::PF_R32_FLOAT);
	Destination->ClearBufferWithFloatSync(0);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Source, TEXT("StoreLastSignal"));

	Source->CopyToBuffer(Destination, 8, 0, 0, Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitCopy(this, Source, [this, Destination]()
		{
			TArray<float> Output;
			Output.AddZeroed(2);

			Destination->MapReadAndExecuteSync([&Output](const void* Data)
				{
					FMemory::Memcpy(Output.GetData(), Data, sizeof(float) * 2);
					return true;
				});

			TestEqual(TEXT("Output[0]"), Output[0], 17.0f);
			TestEqual(TEXT("Output[1]"), Output[1], 17.0f);

		}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCopyTest_SimpleSync, "Compushady.Copy.SimpleSync", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCopyTest_SimpleSync::RunTest(const FString& Parameters)
{
	FString ErrorMessages;

	UCompushadyUAV* Source = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 8, EPixelFormat::PF_R32_FLOAT);
	Source->ClearBufferWithFloatSync(17);

	UCompushadyUAV* Destination = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName + "_", 8, EPixelFormat::PF_R32_FLOAT);
	Destination->ClearBufferWithFloatSync(0);

	Source->CopyToBufferSync(Destination, 8, 0, 0, ErrorMessages);

	TArray<float> Output;
	Output.AddZeroed(2);

	Destination->MapReadAndExecuteSync([&Output](const void* Data)
		{
			FMemory::Memcpy(Output.GetData(), Data, sizeof(float) * 2);
			return true;
		});

	TestEqual(TEXT("Output[0]"), Output[0], 17.0f);
	TestEqual(TEXT("Output[1]"), Output[1], 17.0f);

	return true;
}

#endif
