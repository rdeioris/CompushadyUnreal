// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadyCanvas.h"

UCompushadyCanvas* UCompushadyCanvas::CreateCompushadyCanvas()
{
	// Transient outer; the blitter subsystem strong-refs it once AddCanvas'd.
	return NewObject<UCompushadyCanvas>();
}

FGuid UCompushadyCanvas::AddText(FVector2D Position, const FString& Text,
	FLinearColor Color, float Scale, bool bShadow, UFont* Font)
{
	FCompushadyCanvasText Item;
	Item.Handle = FGuid::NewGuid();
	Item.Position = Position;
	Item.Text = Text;
	Item.Color = Color;
	Item.Scale = Scale;
	Item.bShadow = bShadow;
	Item.Font = Font;
	TextItems.Add(Item);
	return Item.Handle;
}

bool UCompushadyCanvas::UpdateText(const FGuid& Handle, FVector2D Position, const FString& Text,
	FLinearColor Color, float Scale, bool bShadow, UFont* Font)
{
	for (FCompushadyCanvasText& Item : TextItems)
	{
		if (Item.Handle == Handle)
		{
			Item.Position = Position;
			Item.Text = Text;
			Item.Color = Color;
			Item.Scale = Scale;
			Item.bShadow = bShadow;
			Item.Font = Font;
			return true;
		}
	}
	return false;
}

bool UCompushadyCanvas::RemoveText(const FGuid& Handle)
{
	return TextItems.RemoveAll([&Handle](const FCompushadyCanvasText& Item)
		{
			return Item.Handle == Handle;
		}) > 0;
}

void UCompushadyCanvas::Clear()
{
	TextItems.Reset();
}
