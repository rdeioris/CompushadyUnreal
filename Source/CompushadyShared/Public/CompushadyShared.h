// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Module-local log category (core's LogCompushady is not exported across modules).
DECLARE_LOG_CATEGORY_EXTERN(LogCompushadyShared, Log, All);

class FCompushadySharedModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
