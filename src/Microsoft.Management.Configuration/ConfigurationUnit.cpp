// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationUnit.h"
#include "ConfigurationUnit.g.cpp"
#include "ConfigurationSetParser.h"
#include "ConfigurationStatus.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        using ValueSet = Windows::Foundation::Collections::ValueSet;

        ValueSet Clone(const ValueSet& source)
        {
            ValueSet result;

            for (const auto& entry : source)
            {
                ValueSet child = entry.Value().try_as<ValueSet>();

                if (child)
                {
                    result.Insert(entry.Key(), Clone(child));
                }
                else
                {
                    result.Insert(entry.Key(), entry.Value());
                }
            }

            return result;
        }

        Windows::Foundation::Collections::IVector<hstring> Clone(const Windows::Foundation::Collections::IVector<hstring>& value)
        {
            std::vector<hstring> temp{ value.Size() };
            value.GetMany(0, temp);
            return winrt::multi_threaded_vector<hstring>(std::move(temp));
        }
    }

    ConfigurationUnit::ConfigurationUnit()
    {
        GUID instanceIdentifier;
        THROW_IF_FAILED(CoCreateGuid(&instanceIdentifier));
        m_instanceIdentifier = instanceIdentifier;
    }

    ConfigurationUnit::ConfigurationUnit(const guid& instanceIdentifier) :
        m_instanceIdentifier(instanceIdentifier)
    {
    }

    hstring ConfigurationUnit::Type()
    {
        return m_type;
    }

    void ConfigurationUnit::Type(const hstring& value)
    {
        m_type = value;
    }

    guid ConfigurationUnit::InstanceIdentifier()
    {
        return m_instanceIdentifier;
    }

    hstring ConfigurationUnit::Identifier()
    {
        return m_identifier;
    }

    void ConfigurationUnit::Identifier(const hstring& value)
    {
        m_identifier = value;
    }

    ConfigurationUnitIntent ConfigurationUnit::Intent()
    {
        return m_intent;
    }

    void ConfigurationUnit::Intent(ConfigurationUnitIntent value)
    {
        m_intent = value;
    }

    Windows::Foundation::Collections::IVector<hstring> ConfigurationUnit::Dependencies()
    {
        return m_dependencies;
    }

    void ConfigurationUnit::Dependencies(const Windows::Foundation::Collections::IVector<hstring>& value)
    {
        THROW_HR_IF(E_POINTER, !value);
        m_dependencies = value;
    }

    void ConfigurationUnit::Dependencies(std::vector<hstring>&& value)
    {
        m_dependencies = winrt::multi_threaded_vector<hstring>(std::move(value));
    }

    Windows::Foundation::Collections::ValueSet ConfigurationUnit::Metadata()
    {
        return m_metadata;
    }

    void ConfigurationUnit::Metadata(const Windows::Foundation::Collections::ValueSet& value)
    {
        THROW_HR_IF(E_POINTER, !value);
        m_metadata = value;
    }

    Windows::Foundation::Collections::ValueSet ConfigurationUnit::Settings()
    {
        return m_settings;
    }

    void ConfigurationUnit::Settings(const Windows::Foundation::Collections::ValueSet& value)
    {
        THROW_HR_IF(E_POINTER, !value);
        m_settings = value;
    }

    IConfigurationUnitProcessorDetails ConfigurationUnit::Details()
    {
        return m_details;
    }

    void ConfigurationUnit::Details(IConfigurationUnitProcessorDetails&& details)
    {
        m_details = std::move(details);
    }

    ConfigurationUnitState ConfigurationUnit::State()
    {
        auto status = ConfigurationStatus::Instance();
        return status->GetUnitState(m_instanceIdentifier);
    }

    IConfigurationUnitResultInformation ConfigurationUnit::ResultInformation()
    {
        auto status = ConfigurationStatus::Instance();
        return status->GetUnitResultInformation(m_instanceIdentifier);
    }

    bool ConfigurationUnit::IsActive()
    {
        return m_isActive;
    }

    void ConfigurationUnit::IsActive(bool value)
    {
        m_isActive = value;
    }

    Configuration::ConfigurationUnit ConfigurationUnit::Copy()
    {
        auto result = make_self<ConfigurationUnit>();

        result->m_type = m_type;
        result->m_intent = m_intent;
        result->m_dependencies = Clone(m_dependencies);
        result->m_metadata = Clone(m_metadata);
        result->m_settings = Clone(m_settings);
        result->m_details = m_details;
        result->m_environment = make_self<implementation::ConfigurationEnvironment>(*m_environment);

        return *result;
    }

    bool ConfigurationUnit::IsGroup()
    {
        return m_isGroup;
    }

    void ConfigurationUnit::IsGroup(bool value)
    {
        m_isGroup = value;

        if (value)
        {
            if (!m_units)
            {
                m_units = winrt::multi_threaded_vector<Configuration::ConfigurationUnit>();
            }
        }
    }

    Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit> ConfigurationUnit::Units()
    {
        return m_units;
    }

    void ConfigurationUnit::Units(const Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>& value)
    {
        if (m_isGroup)
        {
            THROW_HR_IF(E_POINTER, !value);
        }
        else if (value)
        {
            m_isGroup = true;
        }

        m_units = value;
    }

    void ConfigurationUnit::Units(std::vector<Configuration::ConfigurationUnit>&& value)
    {
        m_units = winrt::multi_threaded_vector<Configuration::ConfigurationUnit>(std::move(value));
    }

    Configuration::ConfigurationEnvironment ConfigurationUnit::Environment()
    {
        return *m_environment;
    }

    implementation::ConfigurationEnvironment& ConfigurationUnit::EnvironmentInternal()
    {
        return *m_environment;
    }

    HRESULT STDMETHODCALLTYPE ConfigurationUnit::SetLifetimeWatcher(IUnknown* watcher)
    {
        return AppInstaller::WinRT::LifetimeWatcherBase::SetLifetimeWatcher(watcher);
    }
}
