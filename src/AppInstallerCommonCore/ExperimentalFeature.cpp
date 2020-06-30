// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "winget/ExperimentalFeature.h"
#include "winget/UserSettings.h"

namespace AppInstaller::Settings
{
    bool ExperimentalFeature::IsEnabled(Feature feature)
    {
        switch (feature)
        {
        case Feature::None:
            return true;
        case Feature::ExperimentalCmd:
            // ExperimentalArg depends on ExperimentalCmd, so instead of failing we could
            // assume that if ExperimentalArg is enabled then ExperimentalCmd is as well.
            return User().Get<Setting::EFExperimentalCmd>() || User().Get<Setting::EFExperimentalArg>();
        case Feature::ExperimentalArg:
            return User().Get<Setting::EFExperimentalArg>();
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    ExperimentalFeature ExperimentalFeature::GetFeature(ExperimentalFeature::Feature feature)
    {
        switch (feature)
        {
        case Feature::ExperimentalCmd:
            return ExperimentalFeature{ "Experimental Command", "experimentalCmd", "https://aka.ms/winget-settings", Feature::ExperimentalCmd };
        case Feature::ExperimentalArg:
            return ExperimentalFeature{ "Experimental Argument", "experimentalArg", "https://aka.ms/winget-settings", Feature::ExperimentalArg };
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<ExperimentalFeature> ExperimentalFeature::GetAllFeatures()
    {
        return
        {
            // Commented out on purpose these are just to provide example on how experimental features work.
            // GetFeature(Feature::ExperimentalCmd),
            // GetFeature(Feature::ExperimentalArg)
        };

    }
}
