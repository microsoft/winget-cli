// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationEnvironment.h"
#include "ConfigurationEnvironment.g.cpp"
#include "ArgumentValidation.h"

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
        DeepCopy(toDeepCopy);
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

    void ConfigurationEnvironment::DeepCopy(const implementation::ConfigurationEnvironment& toDeepCopy)
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

    void ConfigurationEnvironment::ProcessorProperties(const Windows::Foundation::Collections::ValueSet& values)
    {
        std::map<hstring, hstring> properties;

        for (const auto& value : values)
        {
            Windows::Foundation::IPropertyValue property = value.Value().try_as<Windows::Foundation::IPropertyValue>();
            if (property && IsStringableType(property.Type()))
            {
                properties.emplace(value.Key(), ToString(property));
            }
        }

        m_processorProperties = multi_threaded_map(std::move(properties));
    }

    bool ConfigurationEnvironment::IsDefault() const
    {
        return m_context == SecurityContext::Current && m_processorIdentifier.empty() && m_processorProperties.Size() == 0;
    }

    com_ptr<ConfigurationEnvironment> ConfigurationEnvironment::CalculateCommonEnvironment(const std::vector<IConfigurationEnvironmentView>& environments)
    {
        com_ptr<ConfigurationEnvironment> result;

        if (environments.empty())
        {
            result = make_self<ConfigurationEnvironment>();
        }
        else
        {
            result = make_self<ConfigurationEnvironment>(environments.front());

            for (size_t i = 1; i < environments.size(); ++i)
            {
               const IConfigurationEnvironmentView& environment = environments[i];

               if (result->m_context != environment.Context())
               {
                   result->m_context = SecurityContext::Current;
               }

               if (result->m_processorIdentifier != environment.ProcessorIdentifier())
               {
                   result->m_processorIdentifier.clear();
               }

               if (!AreEqual(result->m_processorProperties.GetView(), environment.ProcessorPropertiesView()))
               {
                   result->m_processorProperties = single_threaded_map<hstring, hstring>();
               }

               // Check if we have already found everything to be different
               if (result->IsDefault())
               {
                   break;
               }
            }
        }

        return result;
    }

    bool ConfigurationEnvironment::AreEqual(const Windows::Foundation::Collections::IMapView<hstring, hstring>& a, const Windows::Foundation::Collections::IMapView<hstring, hstring>& b)
    {
        uint32_t a_size = a ? a.Size() : 0;
        uint32_t b_size = b ? b.Size() : 0;

        if (a_size == 0 && b_size == 0)
        {
            return true;
        }
        else if (a_size != b_size)
        {
            return false;
        }

        for (const auto& entry : a)
        {
            hstring key = entry.Key();
            if (!b.HasKey(key) || entry.Value() != b.Lookup(key))
            {
                return false;
            }
        }

        return true;
    }
}
