// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/NetworkSettings.h"
#include "winget/AdminSettings.h"
#include "AppInstallerLogging.h"

namespace AppInstaller::Settings
{
    void NetworkSettings::SetProxyUri(const std::optional<std::string>& proxyUri)
    {
        AICLI_LOG(Core, Info, << "Setting proxy");

        if (proxyUri)
        {
            m_proxyUri = proxyUri.value();
            AICLI_LOG(Core, Info, << "New value for proxy is " << m_proxyUri.value());
        }
        else
        {
            m_proxyUri.reset();
            AICLI_LOG(Core, Info, << "Proxy will not be used");
        }
    }

    InstallerDownloader NetworkSettings::GetInstallerDownloader() const
    {
        // The default is DeliveryOptimization.
        // We use WinINet if specified by settings, or if we want to use proxy (as DO does not support that)
        InstallerDownloader setting = User().Get<Setting::NetworkDownloader>();

        if (m_proxyUri && setting != InstallerDownloader::WinInet)
        {
            AICLI_LOG(Core, Info, << "Forcing use of wininet for download as DO does not support proxy");
            return InstallerDownloader::WinInet;
        }
        else if (setting == InstallerDownloader::Default)
        {
            return InstallerDownloader::DeliveryOptimization;
        }
        else
        {
            return setting;
        }
    }

    NetworkSettings::NetworkSettings()
    {
        // Get the default proxy
        try
        {
            m_proxyUri = GetAdminSetting(StringAdminSetting::DefaultProxy);
        }
        CATCH_LOG()
        AICLI_LOG(Core, Info, << "Default proxy is " << (m_proxyUri ? m_proxyUri.value() : "not set"));
    }

    NetworkSettings& NetworkSettings::Instance()
    {
        static NetworkSettings networkSettings;
        return networkSettings;
    }

    NetworkSettings& Network()
    {
        return NetworkSettings::Instance();
    }
}
