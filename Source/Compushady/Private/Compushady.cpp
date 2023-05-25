// Copyright 2023 - Roberto De Ioris.

#include "Compushady.h"

#define LOCTEXT_NAMESPACE "FCompushadyModule"

DEFINE_LOG_CATEGORY(LogCompushady);

#if WITH_EDITOR
#include "CompushadyShader.h"
#include "CompushadyHLSLSyntaxHighlighter.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailCustomization.h"
#include "PropertyEditorModule.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#endif

#if WITH_EDITOR
class FCompushadyShaderCustomization : public IDetailCustomization
{
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		TArray<TWeakObjectPtr<UObject>> Objects;
		DetailBuilder.GetObjectsBeingCustomized(Objects);

		if (Objects.Num() != 1)
		{
			return;
		}

		TWeakObjectPtr<UCompushadyShader> CompushadyShader = Cast<UCompushadyShader>(Objects[0].Get());

		TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UCompushadyShader, Code));
		DetailBuilder.HideProperty(PropertyHandle);


		IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory("Code");


		CategoryBuilder.AddCustomRow(FText::FromString("Code")).WholeRowContent()[

			SNew(SVerticalBox)
				+ SVerticalBox::Slot().MaxHeight(800)
				[
					SNew(SMultiLineEditableTextBox)
					.AutoWrapText(false)
				.Margin(0.0f)
				.Marshaller(FCompushadyHLSLSyntaxHighlighter::Create())
				.Text(FText::FromString(CompushadyShader->Code))
				.OnTextChanged_Lambda([CompushadyShader](const FText& InCode)
					{
						CompushadyShader->Code = InCode.ToString();
			CompushadyShader->MarkPackageDirty();
					})
				]
			+ SVerticalBox::Slot().FillHeight(0.1)
				[
					SNew(STextBlock).Text(FText::FromString("Test"))
				]

		];
	}

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShared<FCompushadyShaderCustomization>();
	}

};
#endif

void FCompushadyModule::StartupModule()
{
#if WITH_EDITOR
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyModule.RegisterCustomClassLayout(TEXT("CompushadyShader"), FOnGetDetailCustomizationInstance::CreateStatic(&FCompushadyShaderCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
#endif
}

void FCompushadyModule::ShutdownModule()
{
	Compushady::DXCTeardown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCompushadyModule, Compushady)
