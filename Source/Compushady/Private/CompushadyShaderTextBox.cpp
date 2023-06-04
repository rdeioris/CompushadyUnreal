// Copyright 2023 - Roberto De Ioris.


#include "CompushadyShaderTextBox.h"
#include "CompushadyHLSLSyntaxHighlighter.h"

void UCompushadyShaderTextBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	SourceWidget.Reset();
}

TSharedRef<SWidget> UCompushadyShaderTextBox::RebuildWidget()
{
	/*FLuaSyntaxTextStyle Style;
	Style.NormalTextStyle = CodeStyle;
	Style.CommentTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(CommentColor);
	Style.KeywordTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(KeywordColor);
	Style.NilTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(NilColor);
	Style.BasicTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(BasicColor);
	Style.StdLibTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(StdLibColor);
	Style.StringTextStyle = FTextBlockStyle(CodeStyle).SetColorAndOpacity(StringColor);*/

	SourceWidget = SNew(SMultiLineEditableTextBox)

		.Marshaller(FCompushadyHLSLSyntaxHighlighter::Create());
	/*
#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 1
		.Style(&EditableTextBoxStyle)
#else
		.TextStyle(&CodeStyle)
#endif
		.OnKeyCharHandler_UObject(this, &ULuaMultiLineEditableTextBox::OnKeyChar)
		.OnKeyDownHandler_UObject(this, &ULuaMultiLineEditableTextBox::OnKeyDown)
		.IsReadOnly(bIsReadonly)
		.AllowContextMenu(false)
		.OnCursorMoved_UObject(this, &ULuaMultiLineEditableTextBox::OnCursorMoved)
		.Style(&WidgetStyle);*/

	return SourceWidget.ToSharedRef();
}

void UCompushadyShaderTextBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	//EditableTextBoxPtr->SetStyle(&WidgetStyle);

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