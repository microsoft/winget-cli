// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationStaticFunctions.g.h"
#include <winget/IConfigurationStaticsInternals.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationStaticFunctions : ConfigurationStaticFunctionsT<ConfigurationStaticFunctions, winrt::cloaked<AppInstaller::WinRT::IConfigurationStaticsInternals>>
    { 
        ConfigurationStaticFunctions() = default;

        Configuration::ConfigurationUnit CreateConfigurationUnit();
        Configuration::ConfigurationSet CreateConfigurationSet();
        Windows::Foundation::IAsyncOperation<IConfigurationSetProcessorFactory> CreateConfigurationSetProcessorFactoryAsync(hstring const& handler);
        Configuration::ConfigurationProcessor CreateConfigurationProcessor(IConfigurationSetProcessorFactory const& factory);
        bool IsConfigurationAvailable() { return true; }
        Windows::Foundation::IAsyncActionWithProgress<uint32_t> EnsureConfigurationAvailableAsync();
        Configuration::ConfigurationParameter CreateConfigurationParameter();

        // IConfigurationStaticsInternals
        HRESULT STDMETHODCALLTYPE SetExperimentalState(UINT32 state);

    private:
        // By default, enable all state so that in-proc usage contains it.
        AppInstaller::WinRT::ConfigurationStaticsInternalsStateFlags m_state = AppInstaller::WinRT::ConfigurationStaticsInternalsStateFlags::All;
    };
}
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ConfigurationStaticFunctions : ConfigurationStaticFunctionsT<ConfigurationStaticFunctions, implementation::ConfigurationStaticFunctions>
    {
    };
}
