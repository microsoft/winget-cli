// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ApplySettingsResultInstance.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplySettingsResultInstance : ApplySettingsResultInstanceT<ApplySettingsResultInstance>
    {
        ApplySettingsResultInstance();

        bool RebootRequired() const;
        void RebootRequired(bool value);

        Configuration::IConfigurationUnitResultInformation ResultInformation();

    private:
        bool m_rebootRequired = false;
        Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };
}

namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ApplySettingsResultInstance : ApplySettingsResultInstanceT<ApplySettingsResultInstance, implementation::ApplySettingsResultInstance>
    {
    };
}
