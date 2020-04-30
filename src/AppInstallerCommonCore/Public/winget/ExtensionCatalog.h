// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>

#include <filesystem>
#include <string_view>
#include <optional>

namespace AppInstaller::Deployment
{
    using namespace std::string_view_literals;
    constexpr std::wstring_view SourceExtensionName = L"com.microsoft.winget.source"sv;

    constexpr std::wstring_view IndexDBId = L"IndexDB"sv;

    // Wraps an AppExtension.
    struct Extension
    {
        Extension(winrt::Windows::ApplicationModel::AppExtensions::AppExtension extension);

        // Gets the location of the package root.
        std::filesystem::path GetPackagePath() const;

        // Gets the location of the directory shared by the extension.
        std::filesystem::path GetPublicFolderPath() const;

        // Get the version of the package.
        winrt::Windows::ApplicationModel::PackageVersion GetPackageVersion() const;

    private:
        winrt::Windows::ApplicationModel::AppExtensions::AppExtension m_extension;
    };

    // Wraps an AppExtensionCatalog.
    struct ExtensionCatalog
    {
        ExtensionCatalog(std::wstring_view extensionName);

        // Finds an extension by its package family name and id.
        std::optional<Extension> FindByPackageFamilyAndId(std::string_view packageFamilyName, std::wstring_view id) const;

    private:
        winrt::Windows::ApplicationModel::AppExtensions::AppExtensionCatalog m_catalog = nullptr;
    };
}
