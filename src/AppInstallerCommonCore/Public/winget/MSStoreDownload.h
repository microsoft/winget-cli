// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Authentication.h>
#include <AppInstallerArchitecture.h>

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
    };

    struct MSStoreDownloadInfo
    {
        std::vector<MSStoreDownloadFile> MainPackages;
        std::vector<MSStoreDownloadFile> DependencyPackages;
    };

    struct MSStoreDownloadContext
    {
        MSStoreDownloadContext(std::string productId, AppInstaller::Utility::Architecture architecture, std::string locale, AppInstaller::Authentication::AuthenticationArguments authArgs) {};

        // Calls display catalog API and sfs-client to get download info.
        MSStoreDownloadInfo GetDwonloadInfo();

        // Gets license for the corresponding package returned by previous GetDwonloadInfo().
        // GetDwonloadInfo() must be called before calling this method.
        std::vector<BYTE> GetLicense();

    private:
        std::string m_productId;
        std::vector<AppInstaller::Utility::Architecture> m_architectures;
        std::vector<std::string> m_locales;
        //AppInstaller::Authentication::Authenticator m_licensingAuthenticator;
        std::string m_contentId;
    };
}
