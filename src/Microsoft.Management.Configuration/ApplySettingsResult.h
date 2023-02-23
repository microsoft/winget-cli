// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ApplySettingsResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplySettingsResult : ApplySettingsResultT<ApplySettingsResult>
    {
        ApplySettingsResult();

        bool RebootRequired() const;
        void RebootRequired(bool value);

        Configuration::ConfigurationUnitResultInformation ResultInformation();

    private:
        bool m_rebootRequired = false;
        Configuration::ConfigurationUnitResultInformation m_resultInformation;
    };
}

namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ApplySettingsResult : ApplySettingsResultT<ApplySettingsResult, implementation::ApplySettingsResult>
    {
    };
}
