// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationParameter.h"
#include "ConfigurationParameter.g.cpp"
#include "ArgumentValidation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    hstring ConfigurationParameter::Name()
    {
        return m_name;
    }

    void ConfigurationParameter::Name(hstring const& value)
    {
        m_name = value;
    }

    hstring ConfigurationParameter::Description()
    {
        return m_description;
    }

    void ConfigurationParameter::Description(hstring const& value)
    {
        m_description = value;
    }

    Windows::Foundation::Collections::ValueSet ConfigurationParameter::Metadata()
    {
        return m_metadata;
    }

    void ConfigurationParameter::Metadata(const Windows::Foundation::Collections::ValueSet& value)
    {
        THROW_HR_IF(E_POINTER, !value);
        m_metadata = value;
    }

    bool ConfigurationParameter::IsSecure()
    {
        return m_isSecure;
    }

    void ConfigurationParameter::IsSecure(bool value)
    {
        m_isSecure = value;
    }

    Windows::Foundation::PropertyType ConfigurationParameter::Type()
    {
        return m_type;
    }

    void ConfigurationParameter::Type(Windows::Foundation::PropertyType value)
    {
        EnsureSupportedType(value);
        m_type = value;
    }

    Windows::Foundation::IInspectable ConfigurationParameter::DefaultValue()
    {
        return m_defaultValue;
    }

    void ConfigurationParameter::DefaultValue(Windows::Foundation::IInspectable const& value)
    {
        if (value)
        {
            EnsureObjectType(value, m_type);
        }

        m_defaultValue = value;
    }

    Windows::Foundation::Collections::IVector<Windows::Foundation::IInspectable> ConfigurationParameter::AllowedValues()
    {
        return m_allowedValues;
    }

    void ConfigurationParameter::AllowedValues(Windows::Foundation::Collections::IVector<Windows::Foundation::IInspectable> const& value)
    {
        if (value)
        {
            for (const auto& item : value)
            {
                if (item)
                {
                    EnsureObjectType(item, m_type);
                }
            }
        }

        m_allowedValues = value;
    }

    void ConfigurationParameter::AllowedValues(std::vector<Windows::Foundation::IInspectable>&& value)
    {
        m_allowedValues = winrt::multi_threaded_vector<Windows::Foundation::IInspectable>(std::move(value));
    }

    uint32_t ConfigurationParameter::MinimumLength()
    {
        return m_minimumLength;
    }

    void ConfigurationParameter::MinimumLength(uint32_t value)
    {
        EnsureLengthType(m_type);
        m_minimumLength = value;
    }

    uint32_t ConfigurationParameter::MaximumLength()
    {
        return m_maximumLength;
    }

    void ConfigurationParameter::MaximumLength(uint32_t value)
    {
        EnsureLengthType(m_type);
        m_maximumLength = value;
    }

    Windows::Foundation::IInspectable ConfigurationParameter::MinimumValue()
    {
        return m_minimumValue;
    }

    void ConfigurationParameter::MinimumValue(Windows::Foundation::IInspectable const& value)
    {
        if (value)
        {
            EnsureObjectType(value, m_type);
            EnsureComparableType(m_type);
        }

        m_minimumValue = value;
    }

    Windows::Foundation::IInspectable ConfigurationParameter::MaximumValue()
    {
        return m_maximumValue;
    }

    void ConfigurationParameter::MaximumValue(Windows::Foundation::IInspectable const& value)
    {
        if (value)
        {
            EnsureObjectType(value, m_type);
            EnsureComparableType(m_type);
        }

        m_maximumValue = value;
    }

    Windows::Foundation::IInspectable ConfigurationParameter::ProvidedValue()
    {
        return m_providedValue;
    }

    void ConfigurationParameter::ProvidedValue(Windows::Foundation::IInspectable const& value)
    {
        if (value)
        {
            EnsureObjectType(value, m_type);
        }

        m_providedValue = value;
    }

    HRESULT STDMETHODCALLTYPE ConfigurationParameter::SetLifetimeWatcher(IUnknown* watcher)
    {
        return AppInstaller::WinRT::LifetimeWatcherBase::SetLifetimeWatcher(watcher);
    }
}
