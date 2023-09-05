// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSet.g.h"
#include <winget/ILifetimeWatcher.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationSet : ConfigurationSetT<ConfigurationSet, winrt::cloaked<AppInstaller::WinRT::ILifetimeWatcher>>, AppInstaller::WinRT::LifetimeWatcherBase
    {
        using WinRT_Self = ::winrt::Microsoft::Management::Configuration::ConfigurationSet;
        using ConfigurationUnit = ::winrt::Microsoft::Management::Configuration::ConfigurationUnit;

        ConfigurationSet();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        ConfigurationSet(const guid& instanceIdentifier);
        void Initialize(std::vector<Configuration::ConfigurationUnit>&& units);

        bool IsFromHistory() const;
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

        event_token ConfigurationSetChange(const Windows::Foundation::TypedEventHandler<WinRT_Self, ConfigurationSetChangeData>& handler);
        void ConfigurationSetChange(const event_token& token) noexcept;

        void Serialize(const Windows::Storage::Streams::IOutputStream& stream);

        void Remove();

        HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_name;
        hstring m_origin;
        hstring m_path;
        guid m_instanceIdentifier;
        clock::time_point m_firstApply{};
        Windows::Foundation::Collections::IVector<ConfigurationUnit> m_units{ winrt::single_threaded_vector<ConfigurationUnit>() };
        hstring m_schemaVersion;
        winrt::event<Windows::Foundation::TypedEventHandler<WinRT_Self, ConfigurationSetChangeData>> m_configurationSetChange;
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
