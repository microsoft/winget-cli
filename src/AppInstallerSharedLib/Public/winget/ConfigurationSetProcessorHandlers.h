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
    constexpr std::wstring_view DSCv3HandlerIdentifier = L"{dbb2ac6d-1b58-4b05-9c50-b463cc434771}";
    constexpr std::wstring_view DSCv3DynamicRuntimeHandlerIdentifier = L"{5f83e564-ca26-41ca-89db-36f5f0517ffd}";
}
