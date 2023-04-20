// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationUnit.h"
#include "ConfigurationUnit.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ConfigurationUnit::ConfigurationUnit()
    {
        GUID instanceIdentifier;
        THROW_IF_FAILED(CoCreateGuid(&instanceIdentifier));
        m_instanceIdentifier = instanceIdentifier;
    }

    ConfigurationUnit::ConfigurationUnit(const guid& instanceIdentifier) :
        m_instanceIdentifier(instanceIdentifier), m_mutableFlag(false)
    {
    }

    hstring ConfigurationUnit::UnitName()
    {
        return m_unitName;
    }

    void ConfigurationUnit::UnitName(const hstring& value)
    {
        m_mutableFlag.RequireMutable();
        m_unitName = value;
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
        m_mutableFlag.RequireMutable();
        m_identifier = value;
    }

    ConfigurationUnitIntent ConfigurationUnit::Intent()
    {
        return m_intent;
    }

    void ConfigurationUnit::Intent(ConfigurationUnitIntent value)
    {
        m_mutableFlag.RequireMutable();
        m_intent = value;
    }

    Windows::Foundation::Collections::IVectorView<hstring> ConfigurationUnit::Dependencies()
    {
        return m_dependencies.GetView();
    }

    void ConfigurationUnit::Dependencies(const Windows::Foundation::Collections::IVectorView<hstring>& value)
    {
        m_mutableFlag.RequireMutable();

        std::vector<hstring> temp{ value.Size() };
        value.GetMany(0, temp);
        m_dependencies = winrt::single_threaded_vector<hstring>(std::move(temp));
    }

    void ConfigurationUnit::Dependencies(std::vector<hstring>&& value)
    {
        m_dependencies = winrt::single_threaded_vector<hstring>(std::move(value));
    }

    Windows::Foundation::Collections::ValueSet ConfigurationUnit::Directives()
    {
        return m_directives;
    }

    Windows::Foundation::Collections::ValueSet ConfigurationUnit::Settings()
    {
        return m_settings;
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
        return ConfigurationUnitState::Unknown;
    }

    ConfigurationUnitResultInformation ConfigurationUnit::ResultInformation()
    {
        return nullptr;
    }

    bool ConfigurationUnit::ShouldApply()
    {
        return m_shouldApply;
    }

    void ConfigurationUnit::ShouldApply(bool value)
    {
        m_shouldApply = value;
    }
}
