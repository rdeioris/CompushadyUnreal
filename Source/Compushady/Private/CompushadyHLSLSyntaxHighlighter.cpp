// Copyright 2023 - Roberto De Ioris.


#include "CompushadyHLSLSyntaxHighlighter.h"
#include "Runtime/Slate/Public/Framework/Text/IRun.h"
#include "Runtime/Slate/Public/Framework/Text/TextLayout.h"
#include "Runtime/Slate/Public/Framework/Text/SlateTextRun.h"
#include "Widgets/Input/SEditableTextBox.h"

#define ADD_RULE(rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(rule)))

FCompushadyHLSLSyntaxHighlighter::FCompushadyHLSLSyntaxHighlighter(TSharedPtr<FSyntaxTokenizer> InTokenizer) : FSyntaxHighlighterTextLayoutMarshaller(InTokenizer)
{
}

FCompushadyHLSLSyntaxHighlighter::~FCompushadyHLSLSyntaxHighlighter()
{
}

TSharedRef<FCompushadyHLSLSyntaxHighlighter> FCompushadyHLSLSyntaxHighlighter::Create()
{
	TArray<FSyntaxTokenizer::FRule> TokenizerRules;

	// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-appendix-keywords

	// basic types
	ADD_RULE("float");
	ADD_RULE("float1");
	ADD_RULE("float2");
	ADD_RULE("float3");
	ADD_RULE("float4");
	ADD_RULE("float1x1");
	ADD_RULE("float1x2");
	ADD_RULE("float1x3");
	ADD_RULE("float1x4");
	ADD_RULE("float2x1");
	ADD_RULE("float2x2");
	ADD_RULE("float2x3");
	ADD_RULE("float2x4");
	ADD_RULE("float3x1");
	ADD_RULE("float3x2");
	ADD_RULE("float3x3");
	ADD_RULE("float3x4");
	ADD_RULE("float4x1");
	ADD_RULE("float4x2");
	ADD_RULE("float4x3");
	ADD_RULE("float4x4");
	ADD_RULE("half");
	ADD_RULE("int");
	ADD_RULE("matrix");
	ADD_RULE("min16float");
	ADD_RULE("min10float");
	ADD_RULE("min16int");
	ADD_RULE("min12int");
	ADD_RULE("min16uint");
	ADD_RULE("uint");
	ADD_RULE("void");

	// templated types
	ADD_RULE("AppendStructuredBuffer");
	ADD_RULE("Buffer");
	ADD_RULE("ByteAddressBuffer");
	ADD_RULE("AppendStructuredBuffer");
	ADD_RULE("ConsumeStructuredBuffer");
	ADD_RULE("RWBuffer");
	ADD_RULE("RWByteAddressBuffer");
	ADD_RULE("RWStructuredBuffer");
	ADD_RULE("RWTexture1D");
	ADD_RULE("RWTexture1DArray");
	ADD_RULE("RWTexture2D");
	ADD_RULE("RWTexture2DArray");
	ADD_RULE("RWTexture3D");
	ADD_RULE("SamplerState");
	ADD_RULE("SamplerComparisonState");
	ADD_RULE("StructuredBuffer");
	ADD_RULE("Texture1D");
	ADD_RULE("Texture1DArray");
	ADD_RULE("Texture2D");
	ADD_RULE("Texture2DArray");
	ADD_RULE("Texture2DMS");
	ADD_RULE("Texture2DMSArray");
	ADD_RULE("Texture3D");
	ADD_RULE("TextureCube");
	ADD_RULE("TextureCubeArray");

	TokenizerRules.Sort([](const FSyntaxTokenizer::FRule& A, const FSyntaxTokenizer::FRule& B) {
		return A.MatchText.Len() > B.MatchText.Len();
		});


	return MakeShared<FCompushadyHLSLSyntaxHighlighter>(FSyntaxTokenizer::Create(TokenizerRules));
}

void FCompushadyHLSLSyntaxHighlighter::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(TokenizedLines.Num());

	SEditableTextBox::FArguments Defaults;
	FEditableTextBoxStyle WidgetStyle = *Defaults._Style;
	WidgetStyle.BackgroundColor = FSlateColor(FLinearColor::Black);
	WidgetStyle.ForegroundColor = FSlateColor(FLinearColor::White);

	FTextBlockStyle BaseStyle;
	BaseStyle.SetFont(WidgetStyle.TextStyle.Font)
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetSelectedBackgroundColor(FSlateColor(FLinearColor::Blue))
		.SetShadowColorAndOpacity(FLinearColor::Black);

	FTextBlockStyle TokenStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor::Red);

	for (const FSyntaxTokenizer::FTokenizedLine& TokenizedLine : TokenizedLines)
	{
		TSharedRef<FString> ModelString = MakeShared<FString>();
		TArray<TSharedRef<IRun>> Runs;

		for (const FSyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
		{

			FTextBlockStyle CurrentBlockStyle = BaseStyle;

			const FString TokenString = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
			const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenString.Len());

			ModelString->Append(TokenString);

			FRunInfo RunInfo(TEXT("SyntaxHighlight.CompushadyHLSL.Normal"));
			bool bIsWhitespace = FString(TokenString).TrimEnd().IsEmpty();
			if (!bIsWhitespace)
			{
				bool bHasMatchedSyntax = false;
				if (Token.Type == FSyntaxTokenizer::ETokenType::Syntax)
				{

					TCHAR NextChar = TEXT(" ")[0];
					TCHAR PrevChar = TEXT(" ")[0];
					if (Token.Range.EndIndex < SourceString.Len())
					{
						NextChar = SourceString[Token.Range.EndIndex];
					}
					if (Token.Range.BeginIndex > 0)
					{
						PrevChar = SourceString[Token.Range.BeginIndex - 1];
					}

					if (!TChar<WIDECHAR>::IsAlpha(NextChar) && !TChar<WIDECHAR>::IsDigit(NextChar) && !TChar<WIDECHAR>::IsAlpha(PrevChar) && !TChar<WIDECHAR>::IsDigit(PrevChar) && NextChar != TCHAR('_') && PrevChar != TCHAR('_'))
					{
						// do something here
						CurrentBlockStyle = TokenStyle;
					}

				}

				if (Token.Type == FSyntaxTokenizer::ETokenType::Literal || !bHasMatchedSyntax)
				{
					// check for comment or string based on state
				}
				TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, CurrentBlockStyle, ModelRange);
				Runs.Add(Run);
			}
			else
			{
				RunInfo.Name = TEXT("SyntaxHighlight.LuaMachine.WhiteSpace");
				TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, BaseStyle /* use normal style here*/, ModelRange);
				Runs.Add(Run);
			}

		}

		LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));

	}


	TargetTextLayout.AddLines(LinesToAdd);
}