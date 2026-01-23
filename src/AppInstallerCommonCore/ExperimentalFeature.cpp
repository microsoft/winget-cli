// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "winget/ExperimentalFeature.h"
#include "winget/GroupPolicy.h"
#include "winget/UserSettings.h"

namespace AppInstaller::Settings
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    const std::map<ExperimentalFeature::Feature, bool>* s_ExperimentalFeature_Override = nullptr;

    void SetExperimentalFeatureOverride(const std::map<ExperimentalFeature::Feature, bool>* override)
    {
        s_ExperimentalFeature_Override = override;
    }
#endif

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

#ifndef AICLI_DISABLE_TEST_HOOKS
            if (s_ExperimentalFeature_Override)
            {
                auto itr = s_ExperimentalFeature_Override->find(feature);
                if (itr != s_ExperimentalFeature_Override->end())
                {
                    return itr->second;
                }
            }
#endif

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
            case ExperimentalFeature::Feature::DirectMSI:
                return userSettings.Get<Setting::EFDirectMSI>();
            case ExperimentalFeature::Feature::Resume:
                return userSettings.Get<Setting::EFResume>();
            case ExperimentalFeature::Feature::Font:
                return userSettings.Get<Setting::EFFonts>();
            case ExperimentalFeature::Feature::ListDetails:
                return userSettings.Get<Setting::EFListDetails>();
            case ExperimentalFeature::Feature::SourceEdit:
                return userSettings.Get<Setting::EFSourceEdit>();
            case ExperimentalFeature::Feature::SourcePriority:
                return userSettings.Get<Setting::EFSourcePriority>();
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
        case Feature::DirectMSI:
            return ExperimentalFeature{ "Direct MSI Installation", "directMSI", "https://aka.ms/winget-settings", Feature::DirectMSI };
        case Feature::Resume:
            return ExperimentalFeature{ "Resume", "resume", "https://aka.ms/winget-settings", Feature::Resume };
        case Feature::Font:
            return ExperimentalFeature{ "Font", "fonts", "https://aka.ms/winget-settings", Feature::Font };
        case Feature::ListDetails:
            return ExperimentalFeature{ "List Details", "listDetails", "https://aka.ms/winget-settings", Feature::ListDetails };
        case Feature::SourceEdit:
            return ExperimentalFeature{ "Source Editing", "sourceEdit", "https://aka.ms/winget-settings", Feature::SourceEdit };
        case Feature::SourcePriority:
            return ExperimentalFeature{ "Source Priority", "sourcePriority", "https://aka.ms/winget-settings", Feature::SourcePriority };

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
