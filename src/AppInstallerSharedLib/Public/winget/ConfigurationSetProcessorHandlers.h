// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include <winrt/Windows.Foundation.h>
#include <memory>

namespace AppInstaller::Configuration
{
    constexpr std::wstring_view PowerShellHandlerIdentifier = L"pwsh";
    constexpr std::wstring_view DynamicRuntimeHandlerIdentifier = L"{73fea39f-6f4a-41c9-ba94-6fd14d633e40}";
}
