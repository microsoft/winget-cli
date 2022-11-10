// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/AppInstallerMsixInfo.h"

#include <string>
#include <vector>

namespace AppInstaller::Settings
{
    enum class TrustedRemoteSettingFile
    {
        Unknown,
        PackageFile,
        StoreSourceCertPinningConfig,
    };

    struct TrustedRemoteSettings
    {
        static TrustedRemoteSettings const& Instance();

        TrustedRemoteSettings(const TrustedRemoteSettings&) = delete;
        TrustedRemoteSettings& operator=(const TrustedRemoteSettings&) = delete;

        TrustedRemoteSettings(TrustedRemoteSettings&&) = delete;
        TrustedRemoteSettings& operator=(TrustedRemoteSettings&&) = delete;

        std::vector<uint8_t> GetFileContent(TrustedRemoteSettingFile setting, std::string_view packageFile = {}) const;

    private:
        TrustedRemoteSettings();
        ~TrustedRemoteSettings() = default;

        std::unique_ptr<Msix::MsixInfo> m_msixInfo;
    };

    TrustedRemoteSettings const& TrustedRemote();
}
