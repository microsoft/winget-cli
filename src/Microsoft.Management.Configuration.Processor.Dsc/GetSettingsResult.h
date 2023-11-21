// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    struct GetSettingsResult : winrt::implements<GetSettingsResult, winrt::Microsoft::Management::Configuration::IGetSettingsResult>
    {
        GetSettingsResult(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit();

        Windows::Foundation::Collections::ValueSet Settings();
        void Settings(Windows::Foundation::Collections::ValueSet settings);

        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation ResultInformation();
        void ResultInformation(winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation resultInformation);

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        Windows::Foundation::Collections::ValueSet m_settings;
        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };
}
