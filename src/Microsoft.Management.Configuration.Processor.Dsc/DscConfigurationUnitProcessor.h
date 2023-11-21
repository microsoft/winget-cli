// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    struct DscConfigurationUnitProcessor : winrt::implements<DscConfigurationUnitProcessor, winrt::Microsoft::Management::Configuration::IGetAllSettingsConfigurationUnitProcessor>
    {
        DscConfigurationUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit();

        winrt::Microsoft::Management::Configuration::ITestSettingsResult TestSettings();

        winrt::Microsoft::Management::Configuration::IGetSettingsResult GetSettings();

        winrt::Microsoft::Management::Configuration::IApplySettingsResult ApplySettings();

        winrt::Microsoft::Management::Configuration::IGetAllSettingsResult GetAllSettings();

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
    };
}
