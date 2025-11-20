// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string_view>

// Attempts to instantiate all static objects
bool UsePackageManager(std::string_view packageName);
