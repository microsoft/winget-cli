// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "GetSettingsResult.g.h"
#include "winrt/Windows.Foundation.Collections.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct GetSettingsResult : GetSettingsResultT<GetSettingsResult>
    {
        GetSettingsResult();

        Windows::Foundation::Collections::ValueSet Settings();
        void Settings(Windows::Foundation::Collections::ValueSet value);

        Configuration::ConfigurationUnitResultInformation ResultInformation();

    private:
        Windows::Foundation::Collections::ValueSet m_settings = nullptr;
        Configuration::ConfigurationUnitResultInformation m_resultInformation;
    };
}
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct GetSettingsResult : GetSettingsResultT<GetSettingsResult, implementation::GetSettingsResult>
    {
    };
}
