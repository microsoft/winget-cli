// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationEnvironment.h"
#include "ConfigurationEnvironment.g.cpp"
#include "ArgumentValidation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        template <typename T>
        void DeepCopyEnvironmentFrom(implementation::ConfigurationEnvironment& self, const T& toDeepCopy)
        {
            self.Context(toDeepCopy.Context());
            self.ProcessorIdentifier(toDeepCopy.ProcessorIdentifier());
            self.ProcessorProperties(toDeepCopy.ProcessorProperties());
        }
    }

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

    ConfigurationEnvironment::ConfigurationEnvironment(const Configuration::ConfigurationEnvironment& toDeepCopy)
    {
        DeepCopyEnvironmentFrom(*this, toDeepCopy);
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

    void ConfigurationEnvironment::ProcessorIdentifier(const hstring& value)
    {
        m_processorIdentifier = value;
    }

    Windows::Foundation::Collections::IMap<hstring, hstring> ConfigurationEnvironment::ProcessorProperties() const
    {
        return m_processorProperties;
    }

    void ConfigurationEnvironment::DeepCopy(const implementation::ConfigurationEnvironment& toDeepCopy)
    {
        DeepCopyEnvironmentFrom(*this, toDeepCopy);
    }

    void ConfigurationEnvironment::ProcessorProperties(const Windows::Foundation::Collections::IMap<hstring, hstring>& values)
    {
        std::map<hstring, hstring> properties;
        for (const auto& property : values)
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

    com_ptr<ConfigurationEnvironment> ConfigurationEnvironment::CalculateCommonEnvironment(const std::vector<Configuration::ConfigurationEnvironment>& environments)
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
               const Configuration::ConfigurationEnvironment& environment = environments[i];

               if (result->m_context != environment.Context())
               {
                   result->m_context = SecurityContext::Current;
               }

               if (result->m_processorIdentifier != environment.ProcessorIdentifier())
               {
                   result->m_processorIdentifier.clear();
               }

               if (!AreEqual(result->m_processorProperties, environment.ProcessorProperties()))
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

    bool ConfigurationEnvironment::AreEqual(const Windows::Foundation::Collections::IMap<hstring, hstring>& a, const Windows::Foundation::Collections::IMap<hstring, hstring>& b)
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
