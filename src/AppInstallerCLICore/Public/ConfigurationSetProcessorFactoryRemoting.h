// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Windows.h>
#include <winrt/Microsoft.Management.Configuration.h>

namespace AppInstaller::CLI::ConfigurationRemoting
{
    // Creates a factory in another process
    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateOutOfProcessFactory();
}

// Export for use by the out of process factory server to report its initialization.
HRESULT WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(HRESULT result, void* factory, uint64_t memoryHandle, uint64_t initEventHandle, uint64_t completionMutexHandle);
