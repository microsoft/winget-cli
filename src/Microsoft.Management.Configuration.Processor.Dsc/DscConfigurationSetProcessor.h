// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    struct DscConfigurationSetProcessor : winrt::implements<DscConfigurationSetProcessor, winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor>
    {
        winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails GetUnitProcessorDetails(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit,
            winrt::Microsoft::Management::Configuration::ConfigurationUnitDetailFlags detailFlags);

        winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessor CreateUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);
    };
}
