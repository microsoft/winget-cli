// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationEnvironment.h"
#include "ConfigurationEnvironment.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ConfigurationEnvironment::ConfigurationEnvironment() :
        m_processorProperties(multi_threaded_map<hstring, hstring>())
    {}

    ConfigurationEnvironment::ConfigurationEnvironment(SecurityContext context, const std::wstring& processorIdentifier, std::map<hstring, hstring>&& processorProperties) :
        m_context(context), m_processorIdentifier(processorIdentifier), m_processorProperties(multi_threaded_map(std::move(processorProperties)))
    {}

    ConfigurationEnvironment::ConfigurationEnvironment(const implementation::ConfigurationEnvironment& toDeepCopy)
    {
        m_context = toDeepCopy.m_context;
        m_processorIdentifier = toDeepCopy.m_processorIdentifier;

        std::map<hstring, hstring> properties;
        for (const auto& property : toDeepCopy.m_processorProperties)
        {
            properties.emplace(property.Key(), property.Value());
        }
        m_processorProperties = multi_threaded_map(std::move(properties));
    }

    ConfigurationEnvironment::ConfigurationEnvironment(IConfigurationEnvironmentView toDeepCopy)
    {
        m_context = toDeepCopy.Context();
        m_processorIdentifier = toDeepCopy.ProcessorIdentifier();

        std::map<hstring, hstring> properties;
        for(const auto& property : toDeepCopy.ProcessorPropertiesView())
        {
            properties.emplace(property.Key(), property.Value());
        }
        m_processorProperties = multi_threaded_map(std::move(properties));
    }

    SecurityContext ConfigurationEnvironment::Context() const
    {
        return m_context;
    }

    void ConfigurationEnvironment::Context(SecurityContext value)
    {
        m_context = value;
    }

    hstring ConfigurationEnvironment::ProcessorIdentifier() const
    {
        return m_processorIdentifier;
    }

    void ConfigurationEnvironment::ProcessorIdentifier(hstring value)
    {
        m_processorIdentifier = value;
    }

    Windows::Foundation::Collections::IMap<hstring, hstring> ConfigurationEnvironment::ProcessorProperties() const
    {
        return m_processorProperties;
    }

    Windows::Foundation::Collections::IMapView<hstring, hstring> ConfigurationEnvironment::ProcessorPropertiesView() const
    {
        return m_processorProperties.GetView();
    }
}
