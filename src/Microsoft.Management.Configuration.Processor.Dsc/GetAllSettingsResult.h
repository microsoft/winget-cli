// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct GetAllSettingsResult : winrt::implements<GetAllSettingsResult, winrt::Microsoft::Management::Configuration::IGetAllSettingsResult>
    {
        GetAllSettingsResult(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit();

        Windows::Foundation::Collections::IVector<Windows::Foundation::Collections::ValueSet> Settings();

        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation ResultInformation();
        void ResultInformation(winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation resultInformation);

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        Windows::Foundation::Collections::IVector<Windows::Foundation::Collections::ValueSet> m_settings;
        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };
}
