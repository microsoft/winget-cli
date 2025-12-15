// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string_view>
#include "Tests.h"

// Attempts to instantiate all static objects
bool UsePackageManager(const TestParameters& testParameters);

// Sets up the globals for the test caller.
void InitializePackageManagerGlobals();

// Sets the module to prevent it from unloading.
void SetUnloadPreference(bool value);

// Attempts to detect the target package as installed for the system.
bool DetectForSystem(const TestParameters& testParameters);

// Installs the target package for the system.
bool InstallForSystem(const TestParameters& testParameters);
