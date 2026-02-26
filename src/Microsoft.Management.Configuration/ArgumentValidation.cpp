// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#include "ArgumentValidation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void EnsureSupportedType(Windows::Foundation::PropertyType type)
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
        case Windows::Foundation::PropertyType::Boolean:
        case Windows::Foundation::PropertyType::String:
        case Windows::Foundation::PropertyType::Inspectable:
        case Windows::Foundation::PropertyType::DateTime:
        case Windows::Foundation::PropertyType::TimeSpan:
        case Windows::Foundation::PropertyType::Guid:
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
            return;
        }

        THROW_HR(E_INVALIDARG);
    }

    bool IsValidObjectType(Windows::Foundation::IInspectable const& value, Windows::Foundation::PropertyType type)
    {
        auto propertyValue = value.try_as<Windows::Foundation::IPropertyValue>();

        // If the type is an object, it is acceptable for the value to be a ValueSet directly
        if (type == Windows::Foundation::PropertyType::Inspectable &&
            (propertyValue || value.try_as<Windows::Foundation::Collections::ValueSet>()))
        {
            return true;
        }

        // If it wasn't an object type and a ValueSet, it must be an IPropertyValue
        if (!propertyValue)
        {
            return false;
        }

        // If it is an IPropertyValue, it must have the required type
        return (propertyValue.Type() == type);
    }

    void EnsureObjectType(Windows::Foundation::IInspectable const& value, Windows::Foundation::PropertyType type)
    {
        THROW_HR_IF(E_INVALIDARG, !IsValidObjectType(value, type));
    }

    bool IsComparableType(Windows::Foundation::PropertyType type)
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
            return true;
        }

        return false;
    }

    void EnsureComparableType(Windows::Foundation::PropertyType type)
    {
        THROW_HR_IF(E_INVALIDARG, !IsComparableType(type));
    }

    bool IsLengthType(Windows::Foundation::PropertyType type)
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
            return true;
        }

        return false;
    }

    void EnsureLengthType(Windows::Foundation::PropertyType type)
    {
        THROW_HR_IF(E_INVALIDARG, !IsLengthType(type));
    }

    bool IsStringableType(Windows::Foundation::PropertyType type)
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
        case Windows::Foundation::PropertyType::Boolean:
        case Windows::Foundation::PropertyType::String:
            return true;
        }

        return false;
    }

    hstring ToString(Windows::Foundation::IPropertyValue value)
    {
        Windows::Foundation::PropertyType type = value.Type();
        if (type == Windows::Foundation::PropertyType::String)
        {
            return value.GetString();
        }

        std::wostringstream stream;

        switch (value.Type())
        {
        case Windows::Foundation::PropertyType::UInt8:
            stream << value.GetUInt8();
            break;
        case Windows::Foundation::PropertyType::Int16:
            stream << value.GetInt16();
            break;
        case Windows::Foundation::PropertyType::UInt16:
            stream << value.GetUInt16();
            break;
        case Windows::Foundation::PropertyType::Int32:
            stream << value.GetInt32();
            break;
        case Windows::Foundation::PropertyType::UInt32:
            stream << value.GetUInt32();
            break;
        case Windows::Foundation::PropertyType::Int64:
            stream << value.GetInt64();
            break;
        case Windows::Foundation::PropertyType::UInt64:
            stream << value.GetUInt64();
            break;
        case Windows::Foundation::PropertyType::Single:
            stream << value.GetSingle();
            break;
        case Windows::Foundation::PropertyType::Double:
            stream << value.GetDouble();
            break;
        case Windows::Foundation::PropertyType::Char16:
            stream << value.GetChar16();
            break;
        case Windows::Foundation::PropertyType::Boolean:
            stream << value.GetBoolean() ? L"true" : L"false";
            break;
        }

        return hstring{ stream.str() };
    }
}
