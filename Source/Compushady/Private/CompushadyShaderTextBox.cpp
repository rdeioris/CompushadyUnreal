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
	SourceWidget = SNew(SMultiLineEditableTextBox).Marshaller(FCompushadySyntaxHighlighter::CreateHLSL());
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