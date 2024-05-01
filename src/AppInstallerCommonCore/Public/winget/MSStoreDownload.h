// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerArchitecture.h>
#include "winget/Authentication.h"

#include <string>
#include <optional>
#include <string_view>
#include <memory>
#include <vector>

namespace AppInstaller::MSStore
{
    struct MSStoreDownloadFile
    {
        std::string Url;
        std::vector<BYTE> Sha256;
        std::string FileName;
        UINT64 Version = 0;
    };

    struct MSStoreDownloadInfo
    {
        std::vector<MSStoreDownloadFile> MainPackages;
        std::vector<MSStoreDownloadFile> DependencyPackages;
    };

    struct MSStoreDownloadContext
    {
        MSStoreDownloadContext(std::string productId, AppInstaller::Utility::Architecture architecture, std::string locale, AppInstaller::Authentication::AuthenticationArguments authArgs);

        // Calls display catalog API and sfs-client to get download info.
        MSStoreDownloadInfo GetDownloadInfo();

        // Gets license for the corresponding package returned by previous GetDownloadInfo().
        // GetDownloadInfo() must be called before calling this method.
        std::vector<BYTE> GetLicense();

    private:
        std::string m_productId;
        AppInstaller::Utility::Architecture m_architecture = AppInstaller::Utility::Architecture::Unknown;
        std::string m_locale;
        std::unique_ptr<AppInstaller::Authentication::Authenticator> m_licensingAuthenticator;
        std::string m_contentId;
    };
}
