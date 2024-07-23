// Copyright 2023-2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Slate/Public/Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"

#define COMPUSHADY_SH_BEGIN TArray<FSyntaxTokenizer::FRule> TokenizerRules;\
	TSet<FString> TypesTokens;\
	TSet<FString> KeywordsTokens;\
	TSet<FString> PreprocessorsTokens;\
	TSet<FString> IntrinsicsTypesTokens;\
	TSet<FString> SemanticsTokens;\
	TSet<FString> AttributesTokens

#define COMPUSHADY_SH_ADD_TYPE_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule))); TypesTokens.Add(TEXT(Rule))
#define COMPUSHADY_SH_ADD_KEYWORD_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule))); KeywordsTokens.Add(TEXT(Rule))
#define COMPUSHADY_SH_ADD_PREPROCESSOR_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule))); PreprocessorsTokens.Add(TEXT(Rule))
#define COMPUSHADY_SH_ADD_INTRINSIC_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule))); IntrinsicsTypesTokens.Add(TEXT(Rule))
#define COMPUSHADY_SH_ADD_SEMANTIC_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule))); SemanticsTokens.Add(TEXT(Rule))
#define COMPUSHADY_SH_ADD_ATTRIBUTE_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule))); AttributesTokens.Add(TEXT(Rule))
#define COMPUSHADY_SH_ADD_OPEN_COMMENT_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule)))
#define COMPUSHADY_SH_ADD_CLOSE_COMMENT_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule)))
#define COMPUSHADY_SH_ADD_LINE_COMMENT_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule)))
#define COMPUSHADY_SH_ADD_STRING_RULE(Rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(Rule)))
#define COMPUSHADY_SH_END TokenizerRules.Sort([](const FSyntaxTokenizer::FRule& A, const FSyntaxTokenizer::FRule& B)\
	{\
		return A.MatchText.Len() > B.MatchText.Len();\
	});\
	TSharedRef<FCompushadySyntaxHighlighter> SyntaxHighlighter = MakeShared<FCompushadySyntaxHighlighter>(FSyntaxTokenizer::Create(TokenizerRules));\
	SyntaxHighlighter->SetTokensStyle(TypesTokens, &SyntaxHighlighter->GetSyntaxHighlighterConfig().TypeStyle);\
	SyntaxHighlighter->SetTokensStyle(KeywordsTokens, &SyntaxHighlighter->GetSyntaxHighlighterConfig().KeywordStyle);\
	SyntaxHighlighter->SetTokensStyle(PreprocessorsTokens, &SyntaxHighlighter->GetSyntaxHighlighterConfig().PreprocessorStyle);\
	SyntaxHighlighter->SetTokensStyle(IntrinsicsTypesTokens, &SyntaxHighlighter->GetSyntaxHighlighterConfig().IntrinsicStyle);\
	SyntaxHighlighter->SetTokensStyle(SemanticsTokens, &SyntaxHighlighter->GetSyntaxHighlighterConfig().SemanticStyle);\
	SyntaxHighlighter->SetTokensStyle(AttributesTokens, &SyntaxHighlighter->GetSyntaxHighlighterConfig().AttributeStyle);\
	return SyntaxHighlighter;

struct COMPUSHADY_API FCompushadySyntaxHighlighterConfig
{
	FTextBlockStyle BaseStyle;
	FTextBlockStyle TypeStyle;
	FTextBlockStyle KeywordStyle;
	FTextBlockStyle PreprocessorStyle;
	FTextBlockStyle IntrinsicStyle;
	FTextBlockStyle SemanticStyle;
	FTextBlockStyle AttributeStyle;
	FTextBlockStyle CommentStyle;
	FTextBlockStyle StringStyle;

	FCompushadySyntaxHighlighterConfig();
};

/**
 *
 */
class COMPUSHADY_API FCompushadySyntaxHighlighter : public FSyntaxHighlighterTextLayoutMarshaller
{
public:
	FCompushadySyntaxHighlighter(TSharedPtr<FSyntaxTokenizer> InTokenizer);
	~FCompushadySyntaxHighlighter();

	static TSharedRef<FCompushadySyntaxHighlighter> CreateHLSL();
	static TSharedRef<FCompushadySyntaxHighlighter> CreateGLSL();
	static TSharedRef<FCompushadySyntaxHighlighter> CreateWGSL();

	const FCompushadySyntaxHighlighterConfig& GetSyntaxHighlighterConfig() const
	{
		return SyntaxHighlighterConfig;
	}

	void SetTokensStyle(const TSet<FString>& Tokens, const FTextBlockStyle* Style);

protected:
	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines) override;

	FCompushadySyntaxHighlighterConfig SyntaxHighlighterConfig;
	TMap<FString, const FTextBlockStyle*> TokenStylesMap;
};
