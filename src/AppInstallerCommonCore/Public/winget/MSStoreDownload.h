// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerArchitecture.h>
#include <AppInstallerVersions.h>
#include "winget/Authentication.h"
#include "winget/ManifestCommon.h"

#include <string>
#include <optional>
#include <string_view>
#include <memory>
#include <vector>

namespace AppInstaller::MSStore
{
    // Struct representing 1 MSStore package file download info
    struct MSStoreDownloadFile
    {
        std::string Url;
        std::vector<BYTE> Sha256;
        std::string FileName;
        AppInstaller::Utility::UInt64Version Version = 0;
    };

    struct MSStoreDownloadInfo
    {
        std::vector<MSStoreDownloadFile> MainPackages;
        std::vector<MSStoreDownloadFile> DependencyPackages;
    };

    struct MSStoreDownloadContext
    {
        MSStoreDownloadContext(
            std::string productId,
            AppInstaller::Utility::Architecture architecture,
            AppInstaller::Manifest::PlatformEnum platform,
            std::string locale,
            AppInstaller::Authentication::AuthenticationArguments authArgs);

        // Calls display catalog API and sfs-client to get download info.
        MSStoreDownloadInfo GetDownloadInfo();

        // Gets license for the corresponding package returned by previous GetDownloadInfo().
        // GetDownloadInfo() must be called before calling this method.
        std::vector<BYTE> GetLicense();

    private:
        std::string m_productId;
        AppInstaller::Utility::Architecture m_architecture = AppInstaller::Utility::Architecture::Unknown;
        AppInstaller::Manifest::PlatformEnum m_platform = AppInstaller::Manifest::PlatformEnum::Unknown;
        std::string m_locale;
        std::unique_ptr<AppInstaller::Authentication::Authenticator> m_licensingAuthenticator;
        std::string m_contentId;
    };
}
