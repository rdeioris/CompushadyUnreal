// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadyShaderTextBox.h"
#include "CompushadySyntaxHighlighter.h"

void UCompushadyShaderTextBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	SourceWidget.Reset();
}

TSharedRef<SWidget> UCompushadyShaderTextBox::RebuildWidget()
{
	if (ShaderLanguage == ECompushadyShaderLanguage::SPIRV)
	{
		SourceWidget = SNew(SMultiLineEditableTextBox).Marshaller(FCompushadySyntaxHighlighter::CreateSPIRV()).OnKeyCharHandler_UObject(this, &UCompushadyShaderTextBox::OnKeyChar);
	}
	else if (ShaderLanguage == ECompushadyShaderLanguage::GLSL)
	{
		SourceWidget = SNew(SMultiLineEditableTextBox).Marshaller(FCompushadySyntaxHighlighter::CreateGLSL()).OnKeyCharHandler_UObject(this, &UCompushadyShaderTextBox::OnKeyChar);
	}
	else
	{
		SourceWidget = SNew(SMultiLineEditableTextBox).Marshaller(FCompushadySyntaxHighlighter::CreateHLSL()).OnKeyCharHandler_UObject(this, &UCompushadyShaderTextBox::OnKeyChar);
	}
	return SourceWidget.ToSharedRef();
}

void UCompushadyShaderTextBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	Super::SynchronizeTextLayoutProperties(*SourceWidget);
}

#if WITH_EDITOR
const FText UCompushadyShaderTextBox::GetPaletteCategory()
{
	return FText::FromString("Compushady");
}
#endif

FString UCompushadyShaderTextBox::GetSource() const
{
	return SourceWidget->GetText().ToString();
}

void UCompushadyShaderTextBox::SetSource(const FString& Source)
{
	SourceWidget->SetText(FText::FromString(Source));
}

FReply UCompushadyShaderTextBox::OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	const TCHAR Character = InCharacterEvent.GetCharacter();
	if (Character == TEXT('\t'))
	{
		FString Spaces = TEXT("    ");
		SourceWidget->ClearSelection();
		SourceWidget->InsertTextAtCursor(Spaces);
		return FReply::Handled();
	}
	return SourceWidget->OnKeyChar(InGeometry, InCharacterEvent);
}