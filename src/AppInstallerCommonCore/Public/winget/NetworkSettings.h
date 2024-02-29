// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <optional>
#include <string>
#include "winget/UserSettings.h"

namespace AppInstaller::Settings
{
    // Network related settings.
    // Merges information from user settings, admin settings, command line, and group policy.
    struct NetworkSettings
    {
        static NetworkSettings& Instance();

        const std::optional<std::string> GetProxyUri() const { return m_proxyUri; }
        // Sets the proxy URI; may do nothing depending on admin settings and group policy
        void SetProxyUri(const std::optional<std::string>& proxyUri);

        InstallerDownloader GetInstallerDownloader() const;

    protected:
        NetworkSettings();
        ~NetworkSettings() = default;

        std::optional<std::string> m_proxyUri;
    };

    NetworkSettings& Network();
}