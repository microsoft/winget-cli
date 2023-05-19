// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "GetSettingsResultInstance.g.h"
#include "winrt/Windows.Foundation.Collections.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct GetSettingsResultInstance : GetSettingsResultInstanceT<GetSettingsResultInstance>
    {
        GetSettingsResultInstance();

        Windows::Foundation::Collections::ValueSet Settings();
        void Settings(Windows::Foundation::Collections::ValueSet value);

        Configuration::IConfigurationUnitResultInformation ResultInformation();

    private:
        Windows::Foundation::Collections::ValueSet m_settings = nullptr;
        Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };
}
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct GetSettingsResultInstance : GetSettingsResultInstanceT<GetSettingsResultInstance, implementation::GetSettingsResultInstance>
    {
    };
}
