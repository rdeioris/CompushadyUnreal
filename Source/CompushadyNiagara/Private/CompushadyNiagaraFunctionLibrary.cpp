// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadyNiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceCompushady.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "CompushadySRV.h"

UNiagaraDataInterfaceCompushady* UCompushadyNiagaraFunctionLibrary::GetCompushadyDataInterface(UNiagaraComponent* NiagaraComponent, FName ParameterName)
{
	if (!NiagaraComponent)
	{
		return nullptr;
	}

	// Try the name as given, then fall back to the "User." namespaced form.
	UNiagaraDataInterfaceCompushady* DataInterface = UNiagaraFunctionLibrary::GetDataInterface<UNiagaraDataInterfaceCompushady>(NiagaraComponent, ParameterName);
	if (!DataInterface && !ParameterName.ToString().StartsWith(TEXT("User.")))
	{
		const FName PrefixedName(*(FString(TEXT("User.")) + ParameterName.ToString()));
		DataInterface = UNiagaraFunctionLibrary::GetDataInterface<UNiagaraDataInterfaceCompushady>(NiagaraComponent, PrefixedName);
	}

	return DataInterface;
}

bool UCompushadyNiagaraFunctionLibrary::SetCompushadyBuffer(UNiagaraComponent* NiagaraComponent, FName ParameterName, UCompushadySRV* SourceSRV)
{
	UNiagaraDataInterfaceCompushady* DataInterface = GetCompushadyDataInterface(NiagaraComponent, ParameterName);
	if (!DataInterface)
	{
		return false;
	}

	DataInterface->SourceSRV = SourceSRV;

	// Push the change to the render-thread proxy.
	DataInterface->MarkRenderDataDirty();

	return true;
}
