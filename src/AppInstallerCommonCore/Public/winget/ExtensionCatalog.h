// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.ApplicationModel.AppExtensions.h>

#include <string_view>

namespace AppInstaller::Deployment
{
    using namespace std::string_view_literals;
    constexpr std::wstring_view SourceExtensionName = L"com.microsoft.winget.source"sv;

    // Wraps an AppExtensionCatalog.
    struct ExtensionCatalog
    {
        ExtensionCatalog(std::wstring_view extensionName);



    private:
        winrt::Windows::ApplicationModel::AppExtensions::AppExtensionCatalog m_catalog = nullptr;
    };
}
