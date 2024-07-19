// Copyright 2023 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"

class FCompushadyWaitRasterizer : public IAutomationLatentCommand
{
public:
	FCompushadyWaitRasterizer(FAutomationTestBase* InTest, UCompushadyRasterizer* InRasterizer, TFunction<void()> InTestsFunction) : Test(InTest), Rasterizer(InRasterizer), TestsFunction(InTestsFunction)
	{

	}

	bool Update() override
	{
		if (!Rasterizer->IsRunning())
		{
			TestsFunction();
		}
		return !Rasterizer->IsRunning();
	}

private:
	FAutomationTestBase* Test;
	TStrongObjectPtr<UCompushadyRasterizer> Rasterizer;
	TFunction<void()> TestsFunction;
};


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyRasterizerTest_Simple, "Compushady.Rasterizer.Simple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyRasterizerTest_Simple::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString VSCode = "float4 main(const uint vid : SV_VertexID) : SV_Position { if (vid == 0) { return float4(-1, -1, 0, 1); } else if (vid == 1) { return float4(0, 1, 0, 1); } else if (vid == 2) { return float4(1, -1, 0, 1); } return float4(0, 0, 0, 0); }";
	const FString PSCode = "float4 main() : SV_Target0 { return float4(1, 0.5, 0, 1); }";
	UCompushadyRasterizer* Rasterizer = UCompushadyFunctionLibrary::CreateCompushadyVSPSRasterizerFromHLSLString(VSCode, PSCode, FCompushadyRasterizerConfig(), ErrorMessages, "main", "main");


	UE_LOG(LogTemp, Error, TEXT("ErrorMessages: %s"), *ErrorMessages);

	UCompushadyRTV* RTV = UCompushadyFunctionLibrary::CreateCompushadyRTVTexture2D(TestName, 8, 8, EPixelFormat::PF_R8G8B8A8, FLinearColor::Black);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Rasterizer, TEXT("StoreLastSignal"));

	Rasterizer->Draw({}, {}, { RTV }, nullptr, 3, 1, Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitRasterizer(this, Rasterizer, [this, Rasterizer, RTV]()
		{
			TestTrue("Rasterizer->bLastSuccess", Rasterizer->bLastSuccess);

			UE_LOG(LogTemp, Error, TEXT("ErrorMessages: %s"), *(Rasterizer->LastErrorMessages));

			TArray<uint32> Output;
			Output.AddZeroed(8 * 8);

			bool bSuccess = RTV->MapTextureSliceAndExecuteSync([&Output](const void* Data, const int32 RowPitch)
				{

					//CopyTextureData2D(Data, Output.GetData(), 8, EPixelFormat::PF_R8G8B8A8, RowPitch, 8 * sizeof(uint32));
					//FMemory::Memcpy(Output.GetData(), Data, 256);
					UE_LOG(LogTemp, Error, TEXT("Copied"));
				}, 0);

			for (const uint32& Value : Output)
			{
				UE_LOG(LogTemp, Error, TEXT("%x"), Value);
			}

			TestEqual(TEXT("Output[7 * 8]"), Output[7 * 8], 0xff0000ff);
			TestEqual(TEXT("Output[7 * 8 + 7]"), Output[7 * 8 + 7], 0xff0000ff);
			TestEqual(TEXT("Output[3 * 8 + 4]"), Output[3 * 8 + 4], 0xff0000ff);

		}));

	return true;
}

#endif
