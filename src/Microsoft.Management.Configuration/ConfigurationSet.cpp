// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSet.h"
#include "ConfigurationSet.g.cpp"
#include "ConfigurationSetParser.h"

#include <AppInstallerErrors.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ConfigurationSet::ConfigurationSet()
    {
        GUID instanceIdentifier;
        THROW_IF_FAILED(CoCreateGuid(&instanceIdentifier));
        m_instanceIdentifier = instanceIdentifier;
    }

    ConfigurationSet::ConfigurationSet(const Windows::Storage::Streams::IInputStream& stream) : ConfigurationSet()
    {
        bool throwException = true;

        try
        {
            std::unique_ptr<ConfigurationSetParser> parser = ConfigurationSetParser::Create(stream);
            if (parser)
            {
                m_configurationUnits = winrt::single_threaded_vector<ConfigurationUnit>(parser->GetConfigurationUnits());
                throwException = false;
            }
        }
        CATCH_LOG();

        THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, throwException);
    }

    ConfigurationSet::ConfigurationSet(const guid& instanceIdentifier) :
        m_instanceIdentifier(instanceIdentifier), m_mutableFlag(false)
    {
    }

    hstring ConfigurationSet::Name()
    {
        return m_name;
    }

    void ConfigurationSet::Name(const hstring& value)
    {
        m_mutableFlag.RequireMutable();
        m_name = value;
    }

    hstring ConfigurationSet::Origin()
    {
        return m_origin;
    }

    void ConfigurationSet::Origin(const hstring& value)
    {
        m_mutableFlag.RequireMutable();
        m_origin = value;
    }

    hstring ConfigurationSet::Path()
    {
        return m_path;
    }

    void ConfigurationSet::Path(const hstring& value)
    {
        m_mutableFlag.RequireMutable();
        m_path = value;
    }

    guid ConfigurationSet::InstanceIdentifier()
    {
        return m_instanceIdentifier;
    }

    ConfigurationSetState ConfigurationSet::State()
    {
        return ConfigurationSetState::Unknown;
    }

    clock::time_point ConfigurationSet::InitialIntent()
    {
        return clock::time_point{};
    }

    clock::time_point ConfigurationSet::ApplyBegun()
    {
        return clock::time_point{};
    }

    clock::time_point ConfigurationSet::ApplyEnded()
    {
        return clock::time_point{};
    }

    Windows::Foundation::Collections::IVectorView<Configuration::ConfigurationUnit> ConfigurationSet::ConfigurationUnits()
    {
        return m_configurationUnits.GetView();
    }

    void ConfigurationSet::ConfigurationUnits(const Windows::Foundation::Collections::IVectorView<ConfigurationUnit>& value)
    {
        m_mutableFlag.RequireMutable();

        std::vector<ConfigurationUnit> temp{ value.Size() };
        value.GetMany(0, temp);
        m_configurationUnits = winrt::single_threaded_vector<ConfigurationUnit>(std::move(temp));
    }

    event_token ConfigurationSet::ConfigurationSetChange(const Windows::Foundation::TypedEventHandler<WinRT_Self, ConfigurationSetChangeData>& handler)
    {
        return m_configurationSetChange.add(handler);
    }

    void ConfigurationSet::ConfigurationSetChange(const event_token& token) noexcept
    {
        m_configurationSetChange.remove(token);
    }

    void ConfigurationSet::Serialize(const Windows::Storage::Streams::IOutputStream& stream)
    {
        UNREFERENCED_PARAMETER(stream);
        THROW_HR(E_NOTIMPL);
    }

    void ConfigurationSet::Remove()
    {
        THROW_HR(E_NOTIMPL);
    }
}
