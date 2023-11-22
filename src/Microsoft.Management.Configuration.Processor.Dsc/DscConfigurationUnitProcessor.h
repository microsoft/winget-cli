// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>
#include "DscConfigurationSetProcessorFactory.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct DscConfigurationUnitProcessor : winrt::implements<DscConfigurationUnitProcessor, winrt::Microsoft::Management::Configuration::IGetAllSettingsConfigurationUnitProcessor>
    {
        DscConfigurationUnitProcessor(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit, winrt::weak_ref<DscConfigurationSetProcessorFactory> const& weakFactory);

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit();

        winrt::Microsoft::Management::Configuration::ITestSettingsResult TestSettings();

        winrt::Microsoft::Management::Configuration::IGetSettingsResult GetSettings();

        winrt::Microsoft::Management::Configuration::IApplySettingsResult ApplySettings();

        winrt::Microsoft::Management::Configuration::IGetAllSettingsResult GetAllSettings();

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        bool m_isGroup;
        winrt::weak_ref<DscConfigurationSetProcessorFactory> m_weakFactory;
    };
}
