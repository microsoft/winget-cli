// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ApplySettingsResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplySettingsResult : ApplySettingsResultT<ApplySettingsResult>
    {
        ApplySettingsResult();

        Configuration::ConfigurationUnitResultInformation ResultInformation();

    private:
        Configuration::ConfigurationUnitResultInformation m_resultInformation;
    };
}

namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ApplySettingsResult : ApplySettingsResultT<ApplySettingsResult, implementation::ApplySettingsResult>
    {
    };
}
