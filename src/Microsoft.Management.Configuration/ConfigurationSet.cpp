// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSet.h"
#include "ConfigurationSet.g.cpp"
#include "ConfigurationSetParser.h"
#include "ConfigurationSetSerializer.h"
#include "Database/ConfigurationDatabase.h"
#include <AppInstallerLanguageUtilities.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace impl
    {
        struct EnvironmentData
        {
            EnvironmentData(Configuration::ConfigurationEnvironment environment) :
                Environment(environment), Context(environment.Context()), Identifier(environment.ProcessorIdentifier()), Properties(environment.ProcessorProperties())
            {
            }

            EnvironmentData(EnvironmentData&&) = default;

            bool operator==(const EnvironmentData& other) const
            {
                return
                    Context == other.Context &&
                    Identifier == other.Identifier &&
                    ConfigurationEnvironment::AreEqual(Properties, other.Properties);
            }

            Configuration::ConfigurationEnvironment Environment;
            SecurityContext Context;
            hstring Identifier;
            Windows::Foundation::Collections::IMap<hstring, hstring> Properties;
        };

        bool ContainsEnvironment(const std::vector<EnvironmentData>& uniqueEnvironments, const EnvironmentData& data)
        {
            for (const EnvironmentData& item : uniqueEnvironments)
            {
                if (item == data)
                {
                    return true;
                }
            }

            return false;
        }

        void ComputeUniqueEnvironments(std::vector<EnvironmentData>& uniqueEnvironments, const Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>& units)
        {
            for (const Configuration::ConfigurationUnit& unit : units)
            {
                if (unit.IsActive())
                {
                    EnvironmentData data{ unit.Environment() };
                    if (!ContainsEnvironment(uniqueEnvironments, data))
                    {
                        uniqueEnvironments.emplace_back(std::move(data));
                    }

                    if (unit.IsGroup())
                    {
                        ComputeUniqueEnvironments(uniqueEnvironments, unit.Units());
                    }
                }
            }
        }
    }

    ConfigurationSet::ConfigurationSet()
    {
        GUID instanceIdentifier;
        THROW_IF_FAILED(CoCreateGuid(&instanceIdentifier));
        m_instanceIdentifier = instanceIdentifier;
        std::tie(m_schemaVersion, m_schemaUri) = ConfigurationSetParser::LatestVersion();
    }

    ConfigurationSet::ConfigurationSet(const guid& instanceIdentifier) :
        m_instanceIdentifier(instanceIdentifier)
    {
    }

    void ConfigurationSet::Units(std::vector<Configuration::ConfigurationUnit>&& units)
    {
        m_units = winrt::multi_threaded_vector<Configuration::ConfigurationUnit>(std::move(units));
    }

    void ConfigurationSet::Parameters(std::vector<Configuration::ConfigurationParameter>&& value)
    {
        m_parameters = winrt::multi_threaded_vector<Configuration::ConfigurationParameter>(std::move(value));
    }

    hstring ConfigurationSet::Name()
    {
        return m_name;
    }

    void ConfigurationSet::Name(const hstring& value)
    {
        m_name = value;
    }

    hstring ConfigurationSet::Origin()
    {
        return m_origin;
    }

    void ConfigurationSet::Origin(const hstring& value)
    {
        m_origin = value;
    }

    hstring ConfigurationSet::Path()
    {
        return m_path;
    }

    void ConfigurationSet::Path(const hstring& value)
    {
        m_path = value;
    }

    guid ConfigurationSet::InstanceIdentifier() const
    {
        return m_instanceIdentifier;
    }

    ConfigurationSetState ConfigurationSet::State()
    {
        auto status = ConfigurationStatus::Instance();
        return status->GetSetState(m_instanceIdentifier);
    }

    clock::time_point ConfigurationSet::FirstApply()
    {
        auto status = ConfigurationStatus::Instance();
        return status->GetSetFirstApply(m_instanceIdentifier);
    }

    clock::time_point ConfigurationSet::ApplyBegun()
    {
        auto status = ConfigurationStatus::Instance();
        return status->GetSetApplyBegun(m_instanceIdentifier);
    }

    clock::time_point ConfigurationSet::ApplyEnded()
    {
        auto status = ConfigurationStatus::Instance();
        return status->GetSetApplyEnded(m_instanceIdentifier);
    }

    Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit> ConfigurationSet::Units()
    {
        return m_units;
    }

    void ConfigurationSet::Units(const Windows::Foundation::Collections::IVector<ConfigurationUnit>& value)
    {
        THROW_HR_IF(E_POINTER, !value);
        m_units = value;
    }

    hstring ConfigurationSet::SchemaVersion()
    {
        return m_schemaVersion;
    }

    void ConfigurationSet::SchemaVersion(const hstring& value)
    {
        THROW_HR_IF(E_INVALIDARG, !ConfigurationSetParser::IsRecognizedSchemaVersion(value));
        m_schemaUri = ConfigurationSetParser::GetSchemaUriForVersion(value);
        m_schemaVersion = value;
    }

    void ConfigurationSet::ConfigurationSetChange(com_ptr<ConfigurationSetChangeData>& data, const std::optional<guid>& unitInstanceIdentifier) try
    {
        if (unitInstanceIdentifier)
        {
            Windows::Foundation::Collections::IVector<ConfigurationUnit> comUnits = m_units;

            std::vector<ConfigurationUnit> units{ comUnits.Size() };
            units.resize(comUnits.GetMany(0, units));

            for (const ConfigurationUnit& unit : units)
            {
                if (unit.InstanceIdentifier() == unitInstanceIdentifier.value())
                {
                    data->Unit(unit);
                    break;
                }
            }
        }

        m_configurationSetChange(*get_strong(), *data);
    }
    CATCH_LOG();

    event_token ConfigurationSet::ConfigurationSetChange(const Windows::Foundation::TypedEventHandler<WinRT_Self, Configuration::ConfigurationSetChangeData>& handler)
    {
        if (!m_configurationSetChange)
        {
            auto status = ConfigurationStatus::Instance();
            std::atomic_store(&m_setChangeRegistration, status->RegisterForSetChange(*this));
        }

        return m_configurationSetChange.add(handler);
    }

    void ConfigurationSet::ConfigurationSetChange(const event_token& token) noexcept
    {
        m_configurationSetChange.remove(token);

        if (!m_configurationSetChange)
        {
            std::atomic_store(&m_setChangeRegistration, {});
        }
    }

    void ConfigurationSet::Serialize(const Windows::Storage::Streams::IOutputStream& stream)
    {
        std::unique_ptr<ConfigurationSetSerializer> serializer = ConfigurationSetSerializer::CreateSerializer(m_schemaVersion);
        hstring result = serializer->Serialize(this);
        auto resultUtf8 = winrt::to_string(result);
        std::vector<uint8_t> bytes(resultUtf8.begin(), resultUtf8.end());

        Windows::Storage::Streams::DataWriter dataWriter{ stream };
        dataWriter.WriteBytes(bytes);
        dataWriter.StoreAsync().get();
        dataWriter.DetachStream();
    }

    void ConfigurationSet::Remove()
    {
        ConfigurationDatabase database;
        database.EnsureOpened(false);
        database.RemoveSetHistory(*get_strong());
    }

    Windows::Foundation::Collections::ValueSet ConfigurationSet::Metadata()
    {
        return m_metadata;
    }

    void ConfigurationSet::Metadata(const Windows::Foundation::Collections::ValueSet& value)
    {
        THROW_HR_IF(E_POINTER, !value);
        m_metadata = value;
    }

    Windows::Foundation::Collections::IVector<ConfigurationParameter> ConfigurationSet::Parameters()
    {
        return m_parameters;
    }

    void ConfigurationSet::Parameters(const Windows::Foundation::Collections::IVector<ConfigurationParameter>& value)
    {
        THROW_HR_IF(E_POINTER, !value);
        m_parameters = value;
    }

    Windows::Foundation::Collections::ValueSet ConfigurationSet::Variables()
    {
        return m_variables;
    }

    void ConfigurationSet::Variables(const Windows::Foundation::Collections::ValueSet& value)
    {
        THROW_HR_IF(E_POINTER, !value);
        m_variables = value;
    }

    Windows::Foundation::Uri ConfigurationSet::SchemaUri()
    {
        return m_schemaUri;
    }

    void ConfigurationSet::SchemaUri(const Windows::Foundation::Uri& value)
    {
        THROW_HR_IF(E_INVALIDARG, !ConfigurationSetParser::IsRecognizedSchemaUri(value));
        m_schemaVersion = ConfigurationSetParser::GetSchemaVersionForUri(value);
        m_schemaUri = value;
    }

    Configuration::ConfigurationEnvironment ConfigurationSet::Environment()
    {
        return *m_environment;
    }

    implementation::ConfigurationEnvironment& ConfigurationSet::EnvironmentInternal()
    {
        return *m_environment;
    }

    std::vector<Configuration::ConfigurationEnvironment> ConfigurationSet::GetUnitEnvironmentsInternal()
    {
        std::vector<impl::EnvironmentData> uniqueEnvironments;
        ComputeUniqueEnvironments(uniqueEnvironments, m_units);

        std::vector<Configuration::ConfigurationEnvironment> result;
        for (const impl::EnvironmentData& data : uniqueEnvironments)
        {
            result.emplace_back(*make_self<implementation::ConfigurationEnvironment>(data.Environment));
        }
        return result;
    }

    Windows::Foundation::Collections::IVector<Configuration::ConfigurationEnvironment> ConfigurationSet::GetUnitEnvironments()
    {
        return single_threaded_vector(GetUnitEnvironmentsInternal());
    }

    HRESULT STDMETHODCALLTYPE ConfigurationSet::SetLifetimeWatcher(IUnknown* watcher)
    {
        return AppInstaller::WinRT::LifetimeWatcherBase::SetLifetimeWatcher(watcher);
    }

    void ConfigurationSet::SetInputHash(std::string inputHash)
    {
        m_inputHash = std::move(inputHash);
    }

    const std::string& ConfigurationSet::GetInputHash() const
    {
        return m_inputHash;
    }
}
