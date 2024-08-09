// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Windows.h>
#include <winrt/Microsoft.Management.Configuration.h>

namespace AppInstaller::CLI::ConfigurationRemoting
{
    // Creates a factory in another process
    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateOutOfProcessFactory(bool useRunAs = false, const std::string& properties = {}, const std::string& restrictions = {});

    // Creates a factory that can route configurations to the appropriate internal factory.
    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateDynamicRuntimeFactory();
}

// Export for use by the out of process factory server to report its initialization.
HRESULT WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(HRESULT result, void* factory, LPWSTR staticsCallback, LPWSTR completionEventName, DWORD parentProcessId);
