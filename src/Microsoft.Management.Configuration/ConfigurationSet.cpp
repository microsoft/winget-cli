// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSet.h"
#include "ConfigurationSet.g.cpp"

#include <winget/Yaml.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        std::string StreamToString(const Windows::Storage::Streams::IInputStream& stream)
        {
            uint32_t bufferSize = 1 << 20;
            Windows::Storage::Streams::Buffer buffer(bufferSize);
            Windows::Storage::Streams::InputStreamOptions readOptions = Windows::Storage::Streams::InputStreamOptions::Partial | Windows::Storage::Streams::InputStreamOptions::ReadAhead;
            std::string result;

            for (;;)
            {
                Windows::Storage::Streams::IBuffer readBuffer = stream.ReadAsync(buffer, bufferSize, readOptions).GetResults();

                size_t readSize = static_cast<size_t>(readBuffer.Length());
                if (readSize)
                {
                    static_assert(sizeof(char) == sizeof(*readBuffer.data()));
                    result.append(reinterpret_cast<char*>(readBuffer.data()), readSize);
                }
                else
                {
                    break;
                }
            }

            return result;
        }
    }

    ConfigurationSet::ConfigurationSet()
    {
        GUID instanceIdentifier;
        THROW_IF_FAILED(CoCreateGuid(&instanceIdentifier));
        m_instanceIdentifier = instanceIdentifier;
    }

    ConfigurationSet::ConfigurationSet(const Windows::Storage::Streams::IInputStream& stream)
    {
        std::string input = StreamToString(stream);
        AppInstaller::YAML::Node yaml = AppInstaller::YAML::Load(input);
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
