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
        // An unknown processor.
        Unknown,
        // Uses PowerShell DSC v2.
        PowerShell,
        // Uses DSC v3.
        DSCv3,
    };

    std::wstring_view ToString(ProcessorEngine value);

    // Determines the appropriate processor engine to use for the given configuration set.
    ProcessorEngine DetermineProcessorEngine(winrt::Microsoft::Management::Configuration::ConfigurationSet set);

    // Creates a factory in another process
    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateOutOfProcessFactory(ProcessorEngine processorEngine, bool useRunAs = false, const std::string& properties = {}, const std::string& restrictions = {});

    // Creates a factory that can route configurations to the appropriate internal factory.
    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateDynamicRuntimeFactory(ProcessorEngine processorEngine);

    // The property names used with IMap property semantics of remote factories.
    enum class PropertyName
    {
        // The path to the dsc.exe executable.
        // Read / Write
        DscExecutablePath,
        // The path to the dsc.exe executable, as discovered.
        // Read only.
        FoundDscExecutablePath,
        // Whether to request detailed traces from the processor.
        // Read / Write
        DiagnosticTraceEnabled,
        // Getting this value pumps the state machine to determine the best DSC to use.
        // We must respond to the value it returns to properly transition states.
        // Read only.
        FindDscStateMachine,
    };

    // Gets the string for a property name.
    winrt::hstring ToHString(PropertyName name);
}

// Export for use by the out of process factory server to report its initialization.
HRESULT WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(HRESULT result, void* factory, LPWSTR staticsCallback, LPWSTR completionEventName, DWORD parentProcessId);
