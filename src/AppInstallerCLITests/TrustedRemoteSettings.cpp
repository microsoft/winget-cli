// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include "TestHooks.h"

#include <AppInstallerMsixInfo.h>
#include <AppInstallerStrings.h>
#include <winget/TrustedRemoteSettings.h>
#include <winget/Certificates.h>

using namespace TestCommon;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Certificates;
using namespace AppInstaller::Msix;
using namespace AppInstaller::Utility;

namespace
{
    // Override TrustedRemoteSettings using this class.
    // Automatically overrides the trusted remote settings for the lifetime of this object.
    struct TestTrustedRemoteSettings : public AppInstaller::Settings::TrustedRemoteSettings
    {
        TestTrustedRemoteSettings(const std::filesystem::path& settingsFile = {})
        {
            if (!settingsFile.empty())
            {
                std::ifstream file{ settingsFile };
                auto content = ReadEntireStreamAsByteArray(file);
                m_msixInfo = std::make_unique<MsixInfo>(content);
            }

            AppInstaller::Settings::SetTrustedRemoteSettingsOverride(this);
        }

        ~TestTrustedRemoteSettings()
        {
            AppInstaller::Settings::SetTrustedRemoteSettingsOverride(nullptr);
        }
    };
}

TEST_CASE("TrustedRemoteSettings_Certificates_StoreChain", "[certificates]")
{
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFTrustedRemoteSettings>(true);

    SECTION("Trusted Remote Settings is empty")
    {
        TestTrustedRemoteSettings testRemoteSettings;

        PinningConfiguration config;
        REQUIRE_FALSE(config.LoadFromTrustedRemoteSettings());
        REQUIRE(config.IsEmpty());
    }

    SECTION("Trusted Remote Settings with overrided content")
    {
        TestDataFile testRemoteSettingsFile{ "TestTrustedRemoteSettings.msix" };
        TestTrustedRemoteSettings testRemoteSettings{ testRemoteSettingsFile };

        PinningConfiguration config;
        REQUIRE_FALSE(config.LoadFromTrustedRemoteSettings());
        REQUIRE_FALSE(config.IsEmpty());

        PinningDetails details;
        auto leafCertContent = testRemoteSettings.GetFileContent(TrustedRemoteSettingFile::PackageFile, "Certificates\\StoreLeaf1.cer");
        details.LoadCertificate(leafCertContent);
        REQUIRE(config.Validate(details.GetCertificate()));

        auto intermediateCertContent = testRemoteSettings.GetFileContent(TrustedRemoteSettingFile::PackageFile, "Certificates\\StoreIntermediate1.cer");
        details.LoadCertificate(intermediateCertContent);
        REQUIRE_FALSE(config.Validate(details.GetCertificate()));
    }
}