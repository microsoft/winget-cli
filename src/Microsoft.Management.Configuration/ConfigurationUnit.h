// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationUnit.g.h"
#include "MutableFlag.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationUnit : ConfigurationUnitT<ConfigurationUnit>
    {
        ConfigurationUnit();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        ConfigurationUnit(guid instanceIdentifier);
#endif

        hstring UnitName();
        void UnitName(hstring value);

        guid InstanceIdentifier();

        hstring Identifier();
        void Identifier(hstring value);

        Windows::Foundation::Collections::IVectorView<hstring> Dependencies();
        void Dependencies(Windows::Foundation::Collections::IVectorView<hstring> value);

        Windows::Foundation::Collections::ValueSet Directives();

        Windows::Foundation::Collections::ValueSet Settings();
        void Settings(Windows::Foundation::Collections::ValueSet value);

        IConfigurationUnitProcessorDetails Details();

        ConfigurationUnitState State();

        ConfigurationUnitResultInformation ResultInformation();

        bool ShouldApply();
        void ShouldApply(bool value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_unitName;
        guid m_InstanceIdentifier;
        hstring m_identifier;
        Windows::Foundation::Collections::IVector<hstring> m_dependencies{ winrt::single_threaded_vector<hstring>() };
        Windows::Foundation::Collections::ValueSet m_directives;
        Windows::Foundation::Collections::ValueSet m_settings;
        IConfigurationUnitProcessorDetails m_details{ nullptr };
        bool m_shouldApply = true;

        MutableFlag m_mutableFlag;
#endif
    };
}
