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
        m_InstanceIdentifier = instanceIdentifier;
    }

    ConfigurationUnit::ConfigurationUnit(guid instanceIdentifier) :
        m_InstanceIdentifier(instanceIdentifier), m_mutableFlag(false)
    {
    }

    hstring ConfigurationUnit::UnitName()
    {
        return m_unitName;
    }

    void ConfigurationUnit::UnitName(hstring value)
    {
        m_mutableFlag.RequireMutable();
        m_unitName = value;
    }

    guid ConfigurationUnit::InstanceIdentifier()
    {
        return m_InstanceIdentifier;
    }

    hstring ConfigurationUnit::Identifier()
    {
        return m_identifier;
    }

    void ConfigurationUnit::Identifier(hstring value)
    {
        m_mutableFlag.RequireMutable();
        m_identifier = value;
    }

    Windows::Foundation::Collections::IVectorView<hstring> ConfigurationUnit::Dependencies()
    {
        return m_dependencies.GetView();
    }

    void ConfigurationUnit::Dependencies(Windows::Foundation::Collections::IVectorView<hstring> value)
    {
        m_mutableFlag.RequireMutable();
        m_dependencies = winrt::single_threaded_vector<hstring>(value);
    }

    Windows::Foundation::Collections::ValueSet ConfigurationUnit::Directives()
    {
        m_directives.get
    }

    Windows::Foundation::Collections::ValueSet ConfigurationUnit::Settings()
    {

    }

    void ConfigurationUnit::Settings(Windows::Foundation::Collections::ValueSet value)
    {

    }

    IConfigurationUnitProcessorDetails ConfigurationUnit::Details()
    {

    }

    ConfigurationUnitState ConfigurationUnit::State()
    {

    }

    ConfigurationUnitResultInformation ConfigurationUnit::ResultInformation()
    {

    }

    bool ConfigurationUnit::ShouldApply()
    {

    }

    void ConfigurationUnit::ShouldApply(bool value)
    {

    }
}
