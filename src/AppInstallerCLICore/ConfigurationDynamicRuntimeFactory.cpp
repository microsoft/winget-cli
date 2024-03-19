// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include <winget/ILifetimeWatcher.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::Management::Configuration;

namespace AppInstaller::CLI::ConfigurationRemoting
{
    namespace
    {
        // This is implemented completely in the packaged context for now, if we want to make it more configurable, we will probably want to move it to configuration and
        // have this implementation leverage that one with an event handler for the packaged specifics.
        struct DynamicFactory : winrt::implements<DynamicFactory, IConfigurationSetProcessorFactory, winrt::cloaked<WinRT::ILifetimeWatcher>>, WinRT::LifetimeWatcherBase
        {
        };
    }

    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateDynamicRuntimeFactory()
    {
        return winrt::make<DynamicFactory>();
    }
}
