// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Windows.h>
#include <winrt/Microsoft.Management.Configuration.h>

namespace AppInstaller::CLI::ConfigurationRemoting
{
    // The processor engine being used by the factory.
    enum class ProcessorEngine
    {
        // Uses PowerShell DSC v2.
        PowerShell,
        // Uses DSC v3.
        DSCv3,
    };

    // Determines the appropriate processor engine to use for the given configuration set.
    ProcessorEngine DetermineProcessorEngine(winrt::Microsoft::Management::Configuration::ConfigurationSet set);

    // Creates a factory in another process
    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateOutOfProcessFactory(ProcessorEngine processorEngine, bool useRunAs = false, const std::string& properties = {}, const std::string& restrictions = {});

    // Creates a factory that can route configurations to the appropriate internal factory.
    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateDynamicRuntimeFactory(ProcessorEngine processorEngine);
}

// Export for use by the out of process factory server to report its initialization.
HRESULT WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(HRESULT result, void* factory, LPWSTR staticsCallback, LPWSTR completionEventName, DWORD parentProcessId);
