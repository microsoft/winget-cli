// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationEnvironment.g.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationEnvironment : ConfigurationEnvironmentT<ConfigurationEnvironment, IConfigurationEnvironmentView>, AppInstaller::WinRT::ModuleCountBase
    {
        ConfigurationEnvironment();
        ConfigurationEnvironment(SecurityContext context, const std::wstring& processorIdentifier, std::map<hstring, hstring>&& processorProperties);
        ConfigurationEnvironment(const implementation::ConfigurationEnvironment& toDeepCopy);
        ConfigurationEnvironment(IConfigurationEnvironmentView toDeepCopy);

        SecurityContext Context() const;
        void Context(SecurityContext value);

        hstring ProcessorIdentifier() const;
        void ProcessorIdentifier(hstring value);

        Windows::Foundation::Collections::IMap<hstring, hstring> ProcessorProperties() const;
        Windows::Foundation::Collections::IMapView<hstring, hstring> ProcessorPropertiesView() const;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        SecurityContext m_context = SecurityContext::Current;
        hstring m_processorIdentifier;
        Windows::Foundation::Collections::IMap<hstring, hstring> m_processorProperties;
#endif
    };
}
