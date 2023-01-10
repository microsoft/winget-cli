// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSet.h"
#include "ConfigurationSet.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ConfigurationSet::ConfigurationSet()
    {
        GUID instanceIdentifier;
        THROW_IF_FAILED(CoCreateGuid(&instanceIdentifier));
        m_instanceIdentifier = instanceIdentifier;
    }

    ConfigurationSet::ConfigurationSet(const Windows::Storage::Streams::IInputStream& stream)
    {
        UNREFERENCED_PARAMETER(stream);
        THROW_HR(E_NOTIMPL);
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

    Windows::Foundation::Collections::IVectorView<ConfigurationUnit> ConfigurationSet::ConfigurationUnits()
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

    event_token ConfigurationSet::ConfigurationSetChange(const Windows::Foundation::TypedEventHandler<WinRT_Self, ConfigurationSetChangeData>& value)
    {
        return m_configurationSetChange.add(value);
    }

    void ConfigurationSet::ConfigurationSetChange(const event_token& token)
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
