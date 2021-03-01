// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "winget/ExtensionCatalog.h"
#include "AppInstallerErrors.h"
#include "AppInstallerLogging.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::Deployment
{
    namespace AppExt = winrt::Windows::ApplicationModel::AppExtensions;

    Extension::Extension(AppExt::AppExtension extension) : m_extension(extension) {}

    std::filesystem::path Extension::GetPackagePath() const
    {
        return m_extension.Package().InstalledLocation().Path().c_str();
    }

    std::filesystem::path Extension::GetPublicFolderPath() const
    {
        auto folder = m_extension.GetPublicFolderAsync().get();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_EXTENSION_PUBLIC_FAILED, !folder);
        return folder.Path().c_str();
    }

    winrt::Windows::ApplicationModel::PackageVersion Extension::GetPackageVersion() const
    {
        return m_extension.Package().Id().Version();
    }

    bool Extension::VerifyContentIntegrity(IProgressCallback& progress)
    {
        auto operation = m_extension.Package().VerifyContentIntegrityAsync();
        auto removeCancel = progress.SetCancellationFunction([&]() { operation.Cancel(); });
        return operation.get();
    }

    ExtensionCatalog::ExtensionCatalog(std::wstring_view extensionName)
    {
        m_catalog = AppExt::AppExtensionCatalog::Open(winrt::hstring(extensionName));
    }

    std::optional<Extension> ExtensionCatalog::FindByPackageFamilyAndId(std::string_view packageFamilyName, std::wstring_view id) const
    {
        std::wstring wpfn = Utility::ConvertToUTF16(packageFamilyName);
        std::optional<Extension> result;

        auto extensions = m_catalog.FindAllAsync().get();
        for (const auto& extension : extensions)
        {
            auto info = extension.AppInfo();

            AICLI_LOG(Core, Info, << "Examining extension: PFN = " << Utility::ConvertToUTF8(info.PackageFamilyName()) << ", ID = " << Utility::ConvertToUTF8(extension.Id()));

            if (info.PackageFamilyName() == wpfn && extension.Id() == id)
            {
                AICLI_LOG(Core, Info, << "Found matching extension.");
                result = Extension{ extension };
                break;
            }
        }

        if (!result)
        {
            AICLI_LOG(Core, Info, << "Did not find extension: PFN = " << packageFamilyName << ", ID = " << Utility::ConvertToUTF8(id));
        }

        return result;
    }
}
