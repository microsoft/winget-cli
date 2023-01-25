// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationUnit.g.h"
#include "MutableFlag.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationUnit : ConfigurationUnitT<ConfigurationUnit>
    {
        ConfigurationUnit();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        ConfigurationUnit(const guid& instanceIdentifier);
#endif

        hstring UnitName();
        void UnitName(const hstring& value);

        guid InstanceIdentifier();

        hstring Identifier();
        void Identifier(const hstring& value);

        ConfigurationUnitIntent Intent();
        void Intent(ConfigurationUnitIntent value);

        Windows::Foundation::Collections::IVectorView<hstring> Dependencies();
        void Dependencies(const Windows::Foundation::Collections::IVectorView<hstring>& value);

        Windows::Foundation::Collections::ValueSet Directives();

        Windows::Foundation::Collections::ValueSet Settings();

        IConfigurationUnitProcessorDetails Details();

        ConfigurationUnitState State();

        ConfigurationUnitResultInformation ResultInformation();

        bool ShouldApply();
        void ShouldApply(bool value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Dependencies(std::vector<hstring>&& value);
        void Details(IConfigurationUnitProcessorDetails&& details);

    private:
        hstring m_unitName;
        guid m_instanceIdentifier;
        hstring m_identifier;
        ConfigurationUnitIntent m_intent = ConfigurationUnitIntent::Apply;
        Windows::Foundation::Collections::IVector<hstring> m_dependencies{ winrt::single_threaded_vector<hstring>() };
        Windows::Foundation::Collections::ValueSet m_directives;
        Windows::Foundation::Collections::ValueSet m_settings;
        IConfigurationUnitProcessorDetails m_details{ nullptr };
        bool m_shouldApply = true;

        MutableFlag m_mutableFlag;
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