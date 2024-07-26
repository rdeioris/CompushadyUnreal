// Copyright 2024 - Roberto De Ioris.


#include "CompushadyK2Node_MakeHLSLString.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetCompiler.h"
#include "KismetNodes/SGraphNodeK2Base.h"
#include "K2Node_CallFunction.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "CompushadySyntaxHighlighter.h"

FText UCompushadyK2Node_MakeHLSLString::GetMenuCategory() const
{
	return FText::FromString("Compushady");
}

void UCompushadyK2Node_MakeHLSLString::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);
	UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);
		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void UCompushadyK2Node_MakeHLSLString::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_String, NAME_None);
}

FText UCompushadyK2Node_MakeHLSLString::GetNodeTitle(ENodeTitleType::Type Title) const
{
	return FText::FromString("Make HLSL String");
}

FText UCompushadyK2Node_MakeHLSLString::GetTooltipText() const
{
	return FText::FromString("Create an HLSL String with syntax highlighting");
}

FLinearColor UCompushadyK2Node_MakeHLSLString::GetNodeTitleColor() const
{
	return FLinearColor(1, 0, 1);
}

const FString& UCompushadyK2Node_MakeHLSLString::GetShaderString() const
{
	return ShaderString;
}

TSharedPtr<SGraphNode> UCompushadyK2Node_MakeHLSLString::CreateVisualWidget()
{
	class SCompushadyShaderNodeWidget : public SGraphNodeK2Base
	{
	public:
		SLATE_BEGIN_ARGS(SCompushadyShaderNodeWidget)
			{
			}

		SLATE_END_ARGS()

		/** Constructs this widget with InArgs */
		void Construct(const FArguments& InArgs, UK2Node* InNode)
		{
			GraphNode = InNode;
			UpdateGraphNode();
		}

		void CreatePinWidgets() override
		{
			SGraphNodeK2Base::CreatePinWidgets();

			UCompushadyK2Node_MakeHLSLString* MakeHLSLString = Cast<UCompushadyK2Node_MakeHLSLString>(GraphNode);

			LeftNodeBox->AddSlot()
				.AutoHeight()
				[
					SNew(SBox).WidthOverride(500).HeightOverride(700)
						[
							SAssignNew(MakeHLSLString->ShaderEditorWidget, SMultiLineEditableTextBox).Marshaller(FCompushadySyntaxHighlighter::CreateHLSL())
								.Text(FText::FromString(MakeHLSLString->GetShaderString()))
								.OnTextChanged_UObject(MakeHLSLString, &UCompushadyK2Node_MakeHLSLString::UpdateShaderString)
								.OnKeyCharHandler_UObject(MakeHLSLString, &UCompushadyK2Node_MakeHLSLString::OnKeyChar)
						]
				];
		}
	};

	return SNew(SCompushadyShaderNodeWidget, this);
}

void UCompushadyK2Node_MakeHLSLString::UpdateShaderString(const FText& NewText)
{
	Modify();
	ShaderString = NewText.ToString();
	if (HasValidBlueprint())
	{
		UBlueprint* BP = GetBlueprint();
		BP->Status = BS_Dirty;
	}
}

FReply UCompushadyK2Node_MakeHLSLString::OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	const TCHAR Character = InCharacterEvent.GetCharacter();
	if (Character == TEXT('\t'))
	{
		FString Spaces = TEXT("    ");
		ShaderEditorWidget->ClearSelection();
		ShaderEditorWidget->InsertTextAtCursor(Spaces);
		return FReply::Handled();
	}
	return ShaderEditorWidget->OnKeyChar(InGeometry, InCharacterEvent);
}

void UCompushadyK2Node_MakeHLSLString::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralString);
	UK2Node_CallFunction* MakeLiteralStringNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	MakeLiteralStringNode->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(FunctionName));
	MakeLiteralStringNode->AllocateDefaultPins();

	UEdGraphPin* NewInputPin = MakeLiteralStringNode->FindPinChecked(TEXT("Value"));

	NewInputPin->DefaultValue = ShaderString;

	UEdGraphPin* OrgReturnPin = Pins[0];
	UEdGraphPin* MakeLiteralStringReturnPin = MakeLiteralStringNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
	CompilerContext.MovePinLinksToIntermediate(*OrgReturnPin, *MakeLiteralStringReturnPin);

	BreakAllNodeLinks();
}