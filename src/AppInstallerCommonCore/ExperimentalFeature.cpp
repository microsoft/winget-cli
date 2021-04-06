// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
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
            case ExperimentalFeature::Feature::ExperimentalMSStore:
                if (GroupPolicies().GetState(TogglePolicy::Policy::MSStoreSource) == PolicyState::Enabled)
                {
                    // Force enable the feature
                    return true;
                }

                return userSettings.Get<Setting::EFExperimentalMSStore>();
            case ExperimentalFeature::Feature::ExperimentalList:
                return userSettings.Get<Setting::EFList>();
            case ExperimentalFeature::Feature::ExperimentalUpgrade:
                return userSettings.Get<Setting::EFExperimentalUpgrade>();
            case ExperimentalFeature::Feature::ExperimentalUninstall:
                return userSettings.Get<Setting::EFUninstall>();
            case ExperimentalFeature::Feature::ExperimentalImport:
                return userSettings.Get<Setting::EFImport>();
            case ExperimentalFeature::Feature::ExperimentalExport:
                return userSettings.Get<Setting::EFExport>();
            case ExperimentalFeature::Feature::ExperimentalRestSource:
                return userSettings.Get<Setting::EFRestSource>();
            default:
                THROW_HR(E_UNEXPECTED);
            }
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
        case Feature::ExperimentalMSStore:
            return ExperimentalFeature{ "Microsoft Store Support", "experimentalMSStore", "https://aka.ms/winget-settings", Feature::ExperimentalMSStore };
        case Feature::ExperimentalList:
            return ExperimentalFeature{ "List Command", "list", "https://aka.ms/winget-settings", Feature::ExperimentalList };
        case Feature::ExperimentalUpgrade:
            return ExperimentalFeature{ "Upgrade Command", "upgrade", "https://aka.ms/winget-settings", Feature::ExperimentalUpgrade };
        case Feature::ExperimentalUninstall:
            return ExperimentalFeature{ "Uninstall Command", "uninstall", "https://aka.ms/winget-settings", Feature::ExperimentalUninstall };
        case Feature::ExperimentalImport:
            return ExperimentalFeature{ "Import Command", "import", "https://aka.ms/winget-settings", Feature::ExperimentalImport };
        case Feature::ExperimentalExport:
            return ExperimentalFeature{ "Export Command", "export", "https://aka.ms/winget-settings", Feature::ExperimentalExport };
        case Feature::ExperimentalRestSource:
            return ExperimentalFeature{ "Rest Source Support", "restSource", "https://aka.ms/winget-settings", Feature::ExperimentalRestSource };
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
