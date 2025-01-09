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
        // Copies the values from the given environment, including making a new map of the properties.
        void DeepCopy(const implementation::ConfigurationEnvironment& toDeepCopy);

        // Copies the scalar properties from the given ValueSet.
        // Ignores all values that cannot be converted to a string.
        void ProcessorProperties(const Windows::Foundation::Collections::ValueSet& values);

        // Determines if this environment represents the default environment.
        bool IsDefault() const;

        // Creates an environment, setting only fields that are identical between all given environments.
        static com_ptr<ConfigurationEnvironment> CalculateCommonEnvironment(const std::vector<IConfigurationEnvironmentView>& environments);

        // Checks if the two given properties maps are equal.
        static bool AreEqual(const Windows::Foundation::Collections::IMapView<hstring, hstring>& a, const Windows::Foundation::Collections::IMapView<hstring, hstring>& b);

    private:
        SecurityContext m_context = SecurityContext::Current;
        hstring m_processorIdentifier;
        Windows::Foundation::Collections::IMap<hstring, hstring> m_processorProperties;
#endif
    };
}
