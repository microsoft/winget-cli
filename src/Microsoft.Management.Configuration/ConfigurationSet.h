// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSet.g.h"
#include "ConfigurationEnvironment.h"
#include "ConfigurationSetChangeData.h"
#include "ConfigurationStatus.h"
#include <winget/ILifetimeWatcher.h>
#include <winget/ModuleCountBase.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationSet : ConfigurationSetT<ConfigurationSet, winrt::cloaked<AppInstaller::WinRT::ILifetimeWatcher>>, AppInstaller::WinRT::LifetimeWatcherBase, AppInstaller::WinRT::ModuleCountBase
    {
        using WinRT_Self = ::winrt::Microsoft::Management::Configuration::ConfigurationSet;
        using ConfigurationUnit = ::winrt::Microsoft::Management::Configuration::ConfigurationUnit;
        using ConfigurationParameter = ::winrt::Microsoft::Management::Configuration::ConfigurationParameter;

        ConfigurationSet();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        ConfigurationSet(const guid& instanceIdentifier);
        void Units(std::vector<Configuration::ConfigurationUnit>&& units);
        void Parameters(std::vector<Configuration::ConfigurationParameter>&& value);
        void ConfigurationSetChange(com_ptr<ConfigurationSetChangeData>& data, const std::optional<guid>& unitInstanceIdentifier);
        implementation::ConfigurationEnvironment& EnvironmentInternal();
        std::vector<Configuration::ConfigurationEnvironment> GetUnitEnvironmentsInternal();
#endif

        hstring Name();
        void Name(const hstring& value);

        hstring Origin();
        void Origin(const hstring& value);

        hstring Path();
        void Path(const hstring& value);

        guid InstanceIdentifier() const;
        ConfigurationSetState State();
        clock::time_point FirstApply();
        clock::time_point ApplyBegun();
        clock::time_point ApplyEnded();

        Windows::Foundation::Collections::IVector<ConfigurationUnit> Units();
        void Units(const Windows::Foundation::Collections::IVector<ConfigurationUnit>& value);

        hstring SchemaVersion();
        void SchemaVersion(const hstring& value);

        event_token ConfigurationSetChange(const Windows::Foundation::TypedEventHandler<WinRT_Self, Configuration::ConfigurationSetChangeData>& handler);
        void ConfigurationSetChange(const event_token& token) noexcept;

        void Serialize(const Windows::Storage::Streams::IOutputStream& stream);

        void Remove();

        Windows::Foundation::Collections::ValueSet Metadata();
        void Metadata(const Windows::Foundation::Collections::ValueSet& value);

        Windows::Foundation::Collections::IVector<ConfigurationParameter> Parameters();
        void Parameters(const Windows::Foundation::Collections::IVector<ConfigurationParameter>& value);

        Windows::Foundation::Collections::ValueSet Variables();
        void Variables(const Windows::Foundation::Collections::ValueSet& value);

        Windows::Foundation::Uri SchemaUri();
        void SchemaUri(const Windows::Foundation::Uri& value);

        Configuration::ConfigurationEnvironment Environment();

        Windows::Foundation::Collections::IVector<Configuration::ConfigurationEnvironment> GetUnitEnvironments();

        HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void SetInputHash(std::string inputHash);
        const std::string& GetInputHash() const;

    private:
        hstring m_name;
        hstring m_origin;
        hstring m_path;
        guid m_instanceIdentifier;
        clock::time_point m_firstApply{};
        Windows::Foundation::Collections::IVector<ConfigurationUnit> m_units{ winrt::multi_threaded_vector<ConfigurationUnit>() };
        hstring m_schemaVersion;
        winrt::event<Windows::Foundation::TypedEventHandler<WinRT_Self, Configuration::ConfigurationSetChangeData>> m_configurationSetChange;
        Windows::Foundation::Collections::ValueSet m_metadata;
        Windows::Foundation::Collections::IVector<ConfigurationParameter> m_parameters{ winrt::multi_threaded_vector<ConfigurationParameter>() };
        Windows::Foundation::Collections::ValueSet m_variables;
        Windows::Foundation::Uri m_schemaUri = nullptr;
        std::string m_inputHash;
        std::shared_ptr<ConfigurationStatus::SetChangeRegistration> m_setChangeRegistration;
        com_ptr<implementation::ConfigurationEnvironment> m_environment{ make_self<implementation::ConfigurationEnvironment>() };
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ConfigurationSet : ConfigurationSetT<ConfigurationSet, implementation::ConfigurationSet>
    {
    };
}
#endif
