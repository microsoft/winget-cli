// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/TrustedRemoteSettings.h"
#include "Public/winget/Settings.h"
#include "Public/winget/ExperimentalFeature.h"
#include "Public/AppInstallerLogging.h"

namespace AppInstaller::Settings
{
    namespace
    {
        // TODO: Values may need to be updated after server side work is done.
        constexpr static std::string_view s_StoreSourceCertPinningConfigFile = "MSStoreCertConfig.json"sv;
    }

    TrustedRemoteSettings const& TrustedRemoteSettings::Instance()
    {
        static TrustedRemoteSettings trustedRemoteSettings;
        return trustedRemoteSettings;
    }

    TrustedRemoteSettings::TrustedRemoteSettings()
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::TrustedRemoteSettings))
        {
            AICLI_LOG(Core, Info, << "Experimental feature TrustedRemoteSettings is not enabled. Skip initialization.");
            return;
        }

        try
        {
            auto stream = Stream{ Stream::TrustedRemoteSettings }.Get();
            if (stream)
            {
                auto content = Utility::ReadEntireStreamAsByteArray(*stream);
                m_msixInfo = std::make_unique<Msix::MsixInfo>(content);
                THROW_HR_IF_MSG(E_UNEXPECTED, m_msixInfo->GetIsBundle(), "Trusted remote settings should be an msix package.");
            }
        }
        catch (...)
        {
            m_msixInfo.reset();
            AICLI_LOG(Core, Error, << "Failed to initialize trusted remote settings. Settings will be empty.");
        }
    }

    std::vector<uint8_t> TrustedRemoteSettings::GetFileContent(TrustedRemoteSettingFile setting, std::string_view packageFile) const
    {
        THROW_HR_IF(E_INVALIDARG, setting == TrustedRemoteSettingFile::PackageFile && packageFile.empty());

        if (m_msixInfo == nullptr)
        {
            AICLI_LOG(Core, Info, << "Trusted remote setting is empty.");
            return {};
        }

        std::string filePath;
        switch (setting)
        {
        case TrustedRemoteSettingFile::StoreSourceCertPinningConfig:
            filePath = s_StoreSourceCertPinningConfigFile;
            break;
        case TrustedRemoteSettingFile::PackageFile:
            filePath = packageFile;
            break;
        }

        if (filePath.empty())
        {
            AICLI_LOG(Core, Warning, << "Cannot determine package file, returning empty content.");
            return {};
        }

        return m_msixInfo->GetFileContent(filePath);
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static TrustedRemoteSettings* s_TrustedRemoteSettings_Override = nullptr;

    void SetTrustedRemoteSettingsOverride(TrustedRemoteSettings* value)
    {
        s_TrustedRemoteSettings_Override = value;
    }
#endif

    TrustedRemoteSettings const& TrustedRemote()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_TrustedRemoteSettings_Override)
        {
            return *s_TrustedRemoteSettings_Override;
        }
#endif

        return TrustedRemoteSettings::Instance();
    }
}
