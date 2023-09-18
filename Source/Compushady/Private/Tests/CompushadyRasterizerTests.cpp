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
	const FString VSCode = "float4 main(uint vid : SV_VertexID) : SV_Position { if (vid == 0) { return float4(-1, -1, 0, 1); } else if (vid == 1) { return float4(0, 1, 0, 1); } else if (vid == 2) { return float4(1, -1, 0, 1); } return float4(0, 0, 0, 0); }";
	const FString PSCode = "float4 main(float4 pos : SV_position) : SV_Target0 { return float4(1, 0, 0, 1); }";
	UCompushadyRasterizer* Rasterizer = UCompushadyFunctionLibrary::CreateCompushadyVSPSRasterizerFromHLSLString(VSCode, PSCode, ErrorMessages, "main", "main");

	UCompushadyRTV* RTV = UCompushadyFunctionLibrary::CreateCompushadyRTVTexture2D(TestName, 8, 8, EPixelFormat::PF_R8G8B8A8, FLinearColor::Black);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Rasterizer, TEXT("StoreLastSignal"));
	Rasterizer->Draw({}, {}, { RTV }, 3, Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitRasterizer(this, Rasterizer, [this, Rasterizer, RTV]()
		{
			TestTrue("Rasterizer->bLastSuccess", Rasterizer->bLastSuccess);

			TArray<uint32> Output;
			Output.AddZeroed(8 * 8);

			bool bSuccess = RTV->MapTextureSliceAndExecuteSync([&Output](const void* Data, const int32 RowPitch)
				{
					CopyTextureData2D(Data, Output.GetData(), 8, EPixelFormat::PF_R8G8B8A8, RowPitch, 8 * sizeof(uint32));
				}, 0);

			TestEqual(TEXT("Output[7 * 8]"), Output[7 * 8], 0xff0000ff);
			TestEqual(TEXT("Output[7 * 8 + 7]"), Output[7 * 8 + 7], 0xff0000ff);
			TestEqual(TEXT("Output[3 * 8 + 4]"), Output[3 * 8 + 4], 0xff0000ff);

		}));

	return true;
}

#endif
