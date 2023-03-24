// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "winget/ExperimentalFeature.h"
#include "winget/GroupPolicy.h"
#include "winget/UserSettings.h"

namespace AppInstaller::Settings
{
    namespace
    {
        bool IsEnabledInternal(ExperimentalFeature::Feature feature, const UserSettings& userSettings)
        {
            if (feature == ExperimentalFeature::Feature::None)
            {
                return true;
            }

#ifdef WINGET_DISABLE_EXPERIMENTAL_FEATURES
            UNREFERENCED_PARAMETER(userSettings);
            return false;
#else

            if (!GroupPolicies().IsEnabled(TogglePolicy::Policy::ExperimentalFeatures))
            {
                AICLI_LOG(Core, Info, <<
                    "Experimental feature '" << ExperimentalFeature::GetFeature(feature).Name() <<
                    "' is disabled due to group policy: " << TogglePolicy::GetPolicy(TogglePolicy::Policy::ExperimentalFeatures).RegValueName());
                return false;
            }

            switch (feature)
            {
            case ExperimentalFeature::Feature::ExperimentalCmd:
                // ExperimentalArg depends on ExperimentalCmd, so instead of failing we could
                // assume that if ExperimentalArg is enabled then ExperimentalCmd is as well.
                return userSettings.Get<Setting::EFExperimentalCmd>() || userSettings.Get<Setting::EFExperimentalArg>();
            case ExperimentalFeature::Feature::ExperimentalArg:
                return userSettings.Get<Setting::EFExperimentalArg>();
            case ExperimentalFeature::Feature::Dependencies:
                return userSettings.Get<Setting::EFDependencies>();
            case ExperimentalFeature::Feature::DirectMSI:
                return userSettings.Get<Setting::EFDirectMSI>();
            case ExperimentalFeature::Feature::Pinning:
                return userSettings.Get<Setting::EFPinning>();
            case ExperimentalFeature::Feature::UninstallPreviousArgument:
                return userSettings.Get<Setting::EFUninstallPreviousArgument>();
            case ExperimentalFeature::Feature::Configuration:
                return userSettings.Get<Setting::EFConfiguration>();
            case ExperimentalFeature::Feature::WindowsFeature:
                return userSettings.Get<Setting::EFWindowsFeature>();
            default:
                THROW_HR(E_UNEXPECTED);
            }
#endif
        }
    }

    bool ExperimentalFeature::IsEnabled(Feature feature)
    {
        return IsEnabledInternal(feature, User());
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    bool ExperimentalFeature::IsEnabled(Feature feature, const UserSettings& userSettings)
    {
        return IsEnabledInternal(feature, userSettings);
    }
#endif

    ExperimentalFeature ExperimentalFeature::GetFeature(ExperimentalFeature::Feature feature)
    {
        switch (feature)
        {
        case Feature::ExperimentalCmd:
            return ExperimentalFeature{ "Command Sample", "experimentalCmd", "https://aka.ms/winget-settings", Feature::ExperimentalCmd };
        case Feature::ExperimentalArg:
            return ExperimentalFeature{ "Argument Sample", "experimentalArg", "https://aka.ms/winget-settings", Feature::ExperimentalArg };
        case Feature::Dependencies:
            return ExperimentalFeature{ "Show Dependencies Information", "dependencies", "https://aka.ms/winget-settings", Feature::Dependencies };
        case Feature::DirectMSI:
            return ExperimentalFeature{ "Direct MSI Installation", "directMSI", "https://aka.ms/winget-settings", Feature::DirectMSI };
        case Feature::Pinning:
            return ExperimentalFeature{ "Package Pinning", "pinning", "https://aka.ms/winget-settings", Feature::Pinning};
        case Feature::UninstallPreviousArgument:
            return ExperimentalFeature{ "Uninstall Previous Argument", "uninstallPreviousArgument", "https://aka.ms/winget-settings", Feature::UninstallPreviousArgument };
        case Feature::Configuration:
            return ExperimentalFeature{ "Configuration", "configuration", "https://aka.ms/winget-settings#configuration", Feature::Configuration };
        case Feature::WindowsFeature:
            return ExperimentalFeature{ "Windows Feature Dependencies", "windowsFeature", "https://aka.ms/winget-settings", Feature::WindowsFeature };
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<ExperimentalFeature> ExperimentalFeature::GetAllFeatures()
    {
        std::vector<ExperimentalFeature> result;

        for (Feature_t i = 0x1; i < static_cast<Feature_t>(Feature::Max); i = i << 1)
        {
            result.emplace_back(GetFeature(static_cast<Feature>(i)));
        }

        return result;
    }
}
