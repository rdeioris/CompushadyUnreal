// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadySyntaxHighlighter.h"
#include "Runtime/Slate/Public/Framework/Text/IRun.h"
#include "Runtime/Slate/Public/Framework/Text/TextLayout.h"
#include "Runtime/Slate/Public/Framework/Text/SlateTextRun.h"
#include "Widgets/Input/SEditableTextBox.h"

FCompushadySyntaxHighlighterConfig::FCompushadySyntaxHighlighterConfig()
{
	SEditableTextBox::FArguments Defaults;
	FEditableTextBoxStyle WidgetStyle = *Defaults._Style;
	WidgetStyle.BackgroundColor = FSlateColor(FLinearColor::Black);
	WidgetStyle.ForegroundColor = FSlateColor(FLinearColor::White);

	BaseStyle.SetFont(WidgetStyle.TextStyle.Font)
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetSelectedBackgroundColor(FSlateColor(FLinearColor::Blue))
		.SetShadowColorAndOpacity(FLinearColor::Black);

	TypeStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor(0, 0.2, 1));
	KeywordStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor(1, 0, 1));
	CommentStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor::Green);
	StringStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor(1, 0.5, 0));
	PreprocessorStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor(1, 0, 1));
	AttributeStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor::Red);
	IntrinsicStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor(0, 1, 1));
	SemanticStyle = FTextBlockStyle(BaseStyle).SetColorAndOpacity(FLinearColor(1, 1, 0));
}

FCompushadySyntaxHighlighter::FCompushadySyntaxHighlighter(TSharedPtr<FSyntaxTokenizer> InTokenizer) : FSyntaxHighlighterTextLayoutMarshaller(InTokenizer)
{

}

FCompushadySyntaxHighlighter::~FCompushadySyntaxHighlighter()
{
}

void FCompushadySyntaxHighlighter::SetTokensStyle(const TSet<FString>& Tokens, const FTextBlockStyle* Style)
{
	for (const FString& Token : Tokens)
	{
		TokenStylesMap.Add(Token, Style);
	}
}

void FCompushadySyntaxHighlighter::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
	enum class EParseState : uint8
	{
		None,
		LookingForSingleLineComment,
		LookingForMultiLineComment,
		LookingForDoubleQuoteString,
		LookingForWhitespace,
	};

	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(TokenizedLines.Num());

	EParseState ParseState = EParseState::None;

	for (const FSyntaxTokenizer::FTokenizedLine& TokenizedLine : TokenizedLines)
	{
		TSharedRef<FString> ModelString = MakeShared<FString>();
		TArray<TSharedRef<IRun>> Runs;

		if (ParseState == EParseState::LookingForSingleLineComment || ParseState == EParseState::LookingForWhitespace)
		{
			ParseState = EParseState::None;
		}

		for (const FSyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
		{

			const FTextBlockStyle* CurrentBlockStyle = &SyntaxHighlighterConfig.BaseStyle;

			const FString TokenString = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
			const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenString.Len());

			ModelString->Append(TokenString);

			FRunInfo RunInfo(TEXT("SyntaxHighlight.Compushady.Normal"));
			bool bIsWhitespace = FString(TokenString).TrimEnd().IsEmpty();
			if (!bIsWhitespace)
			{
				bool bHasMatchedSyntax = false;
				if (Token.Type == FSyntaxTokenizer::ETokenType::Syntax)
				{
					if (ParseState == EParseState::None)
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

						if (TokenString == TEXT("//"))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.Compushady.Comment");
							CurrentBlockStyle = &SyntaxHighlighterConfig.CommentStyle;
							ParseState = EParseState::LookingForSingleLineComment;
						}
						else if (TokenString == TEXT("/*"))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.Compushady.Comment");
							CurrentBlockStyle = &SyntaxHighlighterConfig.CommentStyle;
							ParseState = EParseState::LookingForMultiLineComment;
						}
						else if (TokenString == TEXT("%"))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.Compushady.Attribute");
							CurrentBlockStyle = &SyntaxHighlighterConfig.AttributeStyle;
							ParseState = EParseState::LookingForWhitespace;
						}
						else if (TokenString == TEXT("\""))
						{
							RunInfo.Name = TEXT("SyntaxHighlight.Compushady.String");
							CurrentBlockStyle = &SyntaxHighlighterConfig.StringStyle;
							ParseState = EParseState::LookingForDoubleQuoteString;
							bHasMatchedSyntax = true;
						}
						else if (!TChar<WIDECHAR>::IsAlpha(NextChar) && !TChar<WIDECHAR>::IsDigit(NextChar) && !TChar<WIDECHAR>::IsAlpha(PrevChar) && !TChar<WIDECHAR>::IsDigit(PrevChar) && NextChar != TCHAR('_') && PrevChar != TCHAR('_'))
						{
							// select the right style
							CurrentBlockStyle = &SyntaxHighlighterConfig.BaseStyle;
							if (TokenStylesMap.Contains(TokenString))
							{
								CurrentBlockStyle = TokenStylesMap[TokenString];
							}
							ParseState = EParseState::None;
						}
					}
					else if (TokenString == TEXT("/*"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.Compushady.Comment");
						CurrentBlockStyle = &SyntaxHighlighterConfig.CommentStyle;
						ParseState = EParseState::LookingForMultiLineComment;
					}
					else if (ParseState == EParseState::LookingForMultiLineComment && TokenString == TEXT("*/"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.Compushady.Comment");
						CurrentBlockStyle = &SyntaxHighlighterConfig.CommentStyle;
						ParseState = EParseState::None;
					}
					else if (ParseState == EParseState::LookingForDoubleQuoteString && TokenString == TEXT("\""))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.Compushady.String");
						CurrentBlockStyle = &SyntaxHighlighterConfig.StringStyle;
						ParseState = EParseState::None;
					}
				}

				if (Token.Type == FSyntaxTokenizer::ETokenType::Literal || !bHasMatchedSyntax)
				{
					if (ParseState == EParseState::LookingForSingleLineComment)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.Compushady.Comment");
						CurrentBlockStyle = &SyntaxHighlighterConfig.CommentStyle;
					}
					else if (ParseState == EParseState::LookingForMultiLineComment)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.Compushady.Comment");
						CurrentBlockStyle = &SyntaxHighlighterConfig.CommentStyle;
					}
					else if (ParseState == EParseState::LookingForWhitespace)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.Compushady.Attribute");
						CurrentBlockStyle = &SyntaxHighlighterConfig.AttributeStyle;
					}
					else if (ParseState == EParseState::LookingForDoubleQuoteString)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.Compushady.String");
						CurrentBlockStyle = &SyntaxHighlighterConfig.StringStyle;
					}
				}
				TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, *CurrentBlockStyle, ModelRange);
				Runs.Add(Run);
			}
			else
			{
				if (ParseState == EParseState::LookingForWhitespace)
				{
					ParseState = EParseState::None;
				}
				RunInfo.Name = TEXT("SyntaxHighlight.Compushady.WhiteSpace");
				TSharedRef<ISlateRun> Run = FSlateTextRun::Create(RunInfo, ModelString, SyntaxHighlighterConfig.BaseStyle /* use normal style here*/, ModelRange);
				Runs.Add(Run);
			}

		}

		LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));

	}


	TargetTextLayout.AddLines(LinesToAdd);
}