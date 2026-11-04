// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/NetworkSettings.h"
#include "winget/AdminSettings.h"
#include "AppInstallerLogging.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::Settings
{
    namespace
    {
        std::string GetProxyUriForLog(std::string_view proxyUri)
        {
            constexpr auto invalidProxyUri = "[invalid proxy URI]";
            if (proxyUri.empty())
            {
                return invalidProxyUri;
            }

            try
            {
                winrt::Windows::Foundation::Uri uri{ Utility::ConvertToUTF16(proxyUri) };
                const auto host = uri.Host();
                // The parser can treat an extra '@' in credentials as part of the host.
                if (host.empty() || std::wstring_view{ host }.find(L'@') != std::wstring_view::npos)
                {
                    return invalidProxyUri;
                }

                std::string result = Utility::ConvertToUTF8(uri.AbsoluteCanonicalUri());
                if (!uri.UserName().empty() || !uri.Password().empty())
                {
                    const auto userInfoStart = uri.SchemeName().size() + 3;
                    const auto userInfoEnd = result.find('@', userInfoStart);
                    if (userInfoEnd == std::string::npos)
                    {
                        return invalidProxyUri;
                    }

                    result.replace(userInfoStart, userInfoEnd - userInfoStart, "[redacted]");
                }
                return result;
            }
            catch (const winrt::hresult_invalid_argument&)
            {
                return invalidProxyUri;
            }
        }
    }

    void NetworkSettings::SetProxyUri(const std::optional<std::string>& proxyUri)
    {
        AICLI_LOG(Core, Info, << "Setting proxy");

        if (proxyUri)
        {
            m_proxyUri = proxyUri.value();
            AICLI_LOG(Core, Info, << "New value for proxy is " << GetProxyUriForLog(m_proxyUri.value()));
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
        AICLI_LOG(Core, Info, << "Default proxy is " << (m_proxyUri ? GetProxyUriForLog(m_proxyUri.value()) : "not set"));
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
