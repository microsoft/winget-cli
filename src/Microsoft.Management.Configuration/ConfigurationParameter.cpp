// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationParameter.h"
#include "ConfigurationParameter.g.cpp"
#include "ArgumentValidation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        // Ensures that the value object matches the expected property type.
        void EnsureObjectType(Windows::Foundation::IInspectable const& value, Windows::Foundation::PropertyType type)
        {
            // If the type is an object, it is acceptable for the value to be a ValueSet directly
            if (type == Windows::Foundation::PropertyType::Inspectable && value.try_as<Windows::Foundation::Collections::ValueSet>())
            {
                return;
            }

            // If it wasn't an object type and a ValueSet, it must be an IPropertyValue
            auto propertyValue = value.try_as<Windows::Foundation::IPropertyValue>();
            THROW_HR_IF(E_INVALIDARG, !propertyValue);

            // If it is an IPropertyValue, it must have the required type
            THROW_HR_IF(E_INVALIDARG, propertyValue.Type() != type);
        }

        // Ensures that the given type supports length restrictions.
        void EnsureLengthType(Windows::Foundation::PropertyType type)
        {
            switch (type)
            {
            case Windows::Foundation::PropertyType::String:
            case Windows::Foundation::PropertyType::UInt8Array:
            case Windows::Foundation::PropertyType::Int16Array:
            case Windows::Foundation::PropertyType::UInt16Array:
            case Windows::Foundation::PropertyType::Int32Array:
            case Windows::Foundation::PropertyType::UInt32Array:
            case Windows::Foundation::PropertyType::Int64Array:
            case Windows::Foundation::PropertyType::UInt64Array:
            case Windows::Foundation::PropertyType::SingleArray:
            case Windows::Foundation::PropertyType::DoubleArray:
            case Windows::Foundation::PropertyType::Char16Array:
            case Windows::Foundation::PropertyType::BooleanArray:
            case Windows::Foundation::PropertyType::StringArray:
            case Windows::Foundation::PropertyType::InspectableArray:
            case Windows::Foundation::PropertyType::DateTimeArray:
            case Windows::Foundation::PropertyType::TimeSpanArray:
            case Windows::Foundation::PropertyType::GuidArray:
            case Windows::Foundation::PropertyType::PointArray:
            case Windows::Foundation::PropertyType::SizeArray:
            case Windows::Foundation::PropertyType::RectArray:
            case Windows::Foundation::PropertyType::OtherTypeArray:
                return;
            }

            THROW_HR(E_INVALIDARG);
        }

        // Ensures that the given type supports comparison.
        void EnsureComparableType(Windows::Foundation::PropertyType type)
        {
            switch (type)
            {
            case Windows::Foundation::PropertyType::UInt8:
            case Windows::Foundation::PropertyType::Int16:
            case Windows::Foundation::PropertyType::UInt16:
            case Windows::Foundation::PropertyType::Int32:
            case Windows::Foundation::PropertyType::UInt32:
            case Windows::Foundation::PropertyType::Int64:
            case Windows::Foundation::PropertyType::UInt64:
            case Windows::Foundation::PropertyType::Single:
            case Windows::Foundation::PropertyType::Double:
            case Windows::Foundation::PropertyType::Char16:
            case Windows::Foundation::PropertyType::DateTime:
            case Windows::Foundation::PropertyType::TimeSpan:
                return;
            }

            THROW_HR(E_INVALIDARG);
        }
    }

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

        m_defaultValue = value;
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
