// Copyright 2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Input/Reply.h"
#include "CompushadyK2Node_MakeStringBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class COMPUSHADYBLUEPRINTNODES_API UCompushadyK2Node_MakeStringBase : public UK2Node
{
	GENERATED_BODY()

public:

	//K2Node implementation
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	//K2Node implementation

	//UEdGraphNode implementation
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool IsNodePure() const override { return true; }
	//UEdGraphNode implementation


	const FString& GetShaderString() const;

	void UpdateShaderString(const FText& NewText);
	FReply OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent);

	virtual TSharedPtr<class ITextLayoutMarshaller> GetSyntaxHighlighter() const;
	virtual const FString& GetShaderLanguage() const;

protected:
	UPROPERTY()
	FString ShaderString;
	
	TSharedPtr<class SMultiLineEditableTextBox> ShaderEditorWidget;
};
