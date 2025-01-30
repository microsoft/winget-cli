// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationUnit.g.h"
#include "ConfigurationEnvironment.h"
#include <winget/ILifetimeWatcher.h>
#include <winget/ModuleCountBase.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationUnit : ConfigurationUnitT<ConfigurationUnit, winrt::cloaked<AppInstaller::WinRT::ILifetimeWatcher>>, AppInstaller::WinRT::LifetimeWatcherBase, AppInstaller::WinRT::ModuleCountBase
    {
        ConfigurationUnit();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        ConfigurationUnit(const guid& instanceIdentifier);

        implementation::ConfigurationEnvironment& EnvironmentInternal();
#endif

        hstring Type();
        void Type(const hstring& value);

        guid InstanceIdentifier();

        hstring Identifier();
        void Identifier(const hstring& value);

        ConfigurationUnitIntent Intent();
        void Intent(ConfigurationUnitIntent value);

        Windows::Foundation::Collections::IVector<hstring> Dependencies();
        void Dependencies(const Windows::Foundation::Collections::IVector<hstring>& value);

        Windows::Foundation::Collections::ValueSet Metadata();
        void Metadata(const Windows::Foundation::Collections::ValueSet& value);

        Windows::Foundation::Collections::ValueSet Settings();
        void Settings(const Windows::Foundation::Collections::ValueSet& value);

        IConfigurationUnitProcessorDetails Details();

        ConfigurationUnitState State();

        IConfigurationUnitResultInformation ResultInformation();

        bool IsActive();
        void IsActive(bool value);

        Configuration::ConfigurationUnit Copy();

        bool IsGroup();
        void IsGroup(bool value);

        Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit> Units();
        void Units(const Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>& value);

        Configuration::ConfigurationEnvironment Environment();

        HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Dependencies(std::vector<hstring>&& value);
        void Details(IConfigurationUnitProcessorDetails&& details);
        void Units(std::vector<Configuration::ConfigurationUnit>&& value);

    private:
        hstring m_type;
        guid m_instanceIdentifier;
        hstring m_identifier;
        ConfigurationUnitIntent m_intent = ConfigurationUnitIntent::Apply;
        Windows::Foundation::Collections::IVector<hstring> m_dependencies{ winrt::multi_threaded_vector<hstring>() };
        Windows::Foundation::Collections::ValueSet m_metadata;
        Windows::Foundation::Collections::ValueSet m_settings;
        IConfigurationUnitProcessorDetails m_details{ nullptr };
        bool m_isActive = true;
        bool m_isGroup = false;
        Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit> m_units = nullptr;
        com_ptr<implementation::ConfigurationEnvironment> m_environment{ make_self<implementation::ConfigurationEnvironment>() };
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ConfigurationUnit : ConfigurationUnitT<ConfigurationUnit, implementation::ConfigurationUnit>
    {
    };
}
#endif
