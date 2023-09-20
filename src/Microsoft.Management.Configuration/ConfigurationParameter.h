// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationParameter.g.h"
#include <winget/ILifetimeWatcher.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <limits>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationParameter : ConfigurationParameterT<ConfigurationParameter, winrt::cloaked<AppInstaller::WinRT::ILifetimeWatcher>>, AppInstaller::WinRT::LifetimeWatcherBase
    {
        ConfigurationParameter() = default;

        hstring Name();
        void Name(hstring const& value);

        hstring Description();
        void Description(hstring const& value);

        Windows::Foundation::Collections::ValueSet Metadata();
        void Metadata(const Windows::Foundation::Collections::ValueSet& value);

        bool IsSecure();
        void IsSecure(bool value);

        Windows::Foundation::PropertyType Type();
        void Type(Windows::Foundation::PropertyType value);

        Windows::Foundation::IInspectable DefaultValue();
        void DefaultValue(Windows::Foundation::IInspectable const& value);

        Windows::Foundation::Collections::IVector<Windows::Foundation::IInspectable> AllowedValues();
        void AllowedValues(Windows::Foundation::Collections::IVector<Windows::Foundation::IInspectable> const& value);

        uint32_t MinimumLength();
        void MinimumLength(uint32_t value);

        uint32_t MaximumLength();
        void MaximumLength(uint32_t value);

        Windows::Foundation::IInspectable MinimumValue();
        void MinimumValue(Windows::Foundation::IInspectable const& value);

        Windows::Foundation::IInspectable MaximumValue();
        void MaximumValue(Windows::Foundation::IInspectable const& value);

        Windows::Foundation::IInspectable ProvidedValue();
        void ProvidedValue(Windows::Foundation::IInspectable const& value);

        HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void AllowedValues(std::vector<Windows::Foundation::IInspectable>&& value);

    private:
        hstring m_name;
        hstring m_description;
        Windows::Foundation::Collections::ValueSet m_metadata;
        bool m_isSecure = false;
        Windows::Foundation::PropertyType m_type = Windows::Foundation::PropertyType::Inspectable;
        Windows::Foundation::IInspectable m_defaultValue;
        Windows::Foundation::Collections::IVector<Windows::Foundation::IInspectable> m_allowedValues;
        uint32_t m_minimumLength = 0;
        uint32_t m_maximumLength = std::numeric_limits<uint32_t>::max();
        Windows::Foundation::IInspectable m_minimumValue;
        Windows::Foundation::IInspectable m_maximumValue;
        Windows::Foundation::IInspectable m_providedValue;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct ConfigurationParameter : ConfigurationParameterT<ConfigurationParameter, implementation::ConfigurationParameter>
    {
    };
}
#endif
