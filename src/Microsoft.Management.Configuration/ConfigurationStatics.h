// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationStatics.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationStatics : ConfigurationStaticsT<ConfigurationStatics>
    { 
        ConfigurationStatics() = default;

        Configuration::ConfigurationUnit CreateConfigurationUnit();
        Configuration::ConfigurationSet CreateConfigurationSet();
        IConfigurationSetProcessorFactory CreateConfigurationSetProcessorFactory(hstring const& handler);
        Configuration::ConfigurationProcessor CreateConfigurationProcessor(IConfigurationSetProcessorFactory const& factory);
    };
}
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ConfigurationStatics : ConfigurationStaticsT<ConfigurationStatics, implementation::ConfigurationStatics>
    {
    };
}
