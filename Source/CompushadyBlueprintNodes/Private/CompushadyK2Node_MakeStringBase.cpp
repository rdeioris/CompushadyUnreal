// Copyright 2024 - Roberto De Ioris.


#include "CompushadyK2Node_MakeStringBase.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetCompiler.h"
#include "KismetNodes/SGraphNodeK2Base.h"
#include "K2Node_CallFunction.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

FText UCompushadyK2Node_MakeStringBase::GetMenuCategory() const
{
	return FText::FromString("Compushady");
}

void UCompushadyK2Node_MakeStringBase::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);
	UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);
		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void UCompushadyK2Node_MakeStringBase::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_String, NAME_None);
}

FText UCompushadyK2Node_MakeStringBase::GetNodeTitle(ENodeTitleType::Type Title) const
{
	return FText::FromString(FString::Printf(TEXT("Make %s String"), *GetShaderLanguage()));
}

FText UCompushadyK2Node_MakeStringBase::GetTooltipText() const
{
	return FText::FromString(FString::Printf(TEXT("Create %s String with syntax highlighting"), *GetShaderLanguage()));
}

FLinearColor UCompushadyK2Node_MakeStringBase::GetNodeTitleColor() const
{
	return FLinearColor(1, 0, 1);
}

const FString& UCompushadyK2Node_MakeStringBase::GetShaderString() const
{
	return ShaderString;
}

TSharedPtr<SGraphNode> UCompushadyK2Node_MakeStringBase::CreateVisualWidget()
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

			UCompushadyK2Node_MakeStringBase* MakeString = Cast<UCompushadyK2Node_MakeStringBase>(GraphNode);

			LeftNodeBox->AddSlot()
				.AutoHeight()
				[
					SNew(SBox).MinDesiredWidth(500).MinDesiredHeight(350).MaxDesiredWidth(1000).MaxDesiredHeight(700)
						[
							SAssignNew(MakeString->ShaderEditorWidget, SMultiLineEditableTextBox).Marshaller(MakeString->GetSyntaxHighlighter())
								.Text(FText::FromString(MakeString->GetShaderString()))
								.OnTextChanged_UObject(MakeString, &UCompushadyK2Node_MakeStringBase::UpdateShaderString)
								.OnKeyCharHandler_UObject(MakeString, &UCompushadyK2Node_MakeStringBase::OnKeyChar)
						]
				];
		}
	};

	return SNew(SCompushadyShaderNodeWidget, this);
}

void UCompushadyK2Node_MakeStringBase::UpdateShaderString(const FText& NewText)
{
	Modify();
	ShaderString = NewText.ToString();
	if (HasValidBlueprint())
	{
		UBlueprint* BP = GetBlueprint();
		BP->Status = BS_Dirty;
	}
}

FReply UCompushadyK2Node_MakeStringBase::OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
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

void UCompushadyK2Node_MakeStringBase::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
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

const FString& UCompushadyK2Node_MakeStringBase::GetShaderLanguage() const
{
	static FString ShaderLanguage = "Unknown";
	return ShaderLanguage;
}

TSharedPtr<ITextLayoutMarshaller> UCompushadyK2Node_MakeStringBase::GetSyntaxHighlighter() const
{
	return nullptr;
}