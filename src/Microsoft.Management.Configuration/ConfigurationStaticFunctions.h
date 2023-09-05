// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationStaticFunctions.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationStaticFunctions : ConfigurationStaticFunctionsT<ConfigurationStaticFunctions>
    { 
        ConfigurationStaticFunctions() = default;

        Configuration::ConfigurationUnit CreateConfigurationUnit();
        Configuration::ConfigurationSet CreateConfigurationSet();
        Windows::Foundation::IAsyncOperation<IConfigurationSetProcessorFactory> CreateConfigurationSetProcessorFactoryAsync(hstring const& handler);
        Configuration::ConfigurationProcessor CreateConfigurationProcessor(IConfigurationSetProcessorFactory const& factory);
        bool IsConfigurationAvailable() { return true; }
        Windows::Foundation::IAsyncActionWithProgress<uint32_t> EnsureConfigurationAvailableAsync();
    };
}
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ConfigurationStaticFunctions : ConfigurationStaticFunctionsT<ConfigurationStaticFunctions, implementation::ConfigurationStaticFunctions>
    {
    };
}
