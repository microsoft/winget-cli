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
        case winrt::Windows::Foundation::PropertyType::UInt8:
        case winrt::Windows::Foundation::PropertyType::Int16:
        case winrt::Windows::Foundation::PropertyType::UInt16:
        case winrt::Windows::Foundation::PropertyType::Int32:
        case winrt::Windows::Foundation::PropertyType::UInt32:
        case winrt::Windows::Foundation::PropertyType::Int64:
        case winrt::Windows::Foundation::PropertyType::UInt64:
        case winrt::Windows::Foundation::PropertyType::Single:
        case winrt::Windows::Foundation::PropertyType::Double:
        case winrt::Windows::Foundation::PropertyType::Char16:
        case winrt::Windows::Foundation::PropertyType::Boolean:
        case winrt::Windows::Foundation::PropertyType::String:
        case winrt::Windows::Foundation::PropertyType::Inspectable:
        case winrt::Windows::Foundation::PropertyType::DateTime:
        case winrt::Windows::Foundation::PropertyType::TimeSpan:
        case winrt::Windows::Foundation::PropertyType::Guid:
        case winrt::Windows::Foundation::PropertyType::UInt8Array:
        case winrt::Windows::Foundation::PropertyType::Int16Array:
        case winrt::Windows::Foundation::PropertyType::UInt16Array:
        case winrt::Windows::Foundation::PropertyType::Int32Array:
        case winrt::Windows::Foundation::PropertyType::UInt32Array:
        case winrt::Windows::Foundation::PropertyType::Int64Array:
        case winrt::Windows::Foundation::PropertyType::UInt64Array:
        case winrt::Windows::Foundation::PropertyType::SingleArray:
        case winrt::Windows::Foundation::PropertyType::DoubleArray:
        case winrt::Windows::Foundation::PropertyType::Char16Array:
        case winrt::Windows::Foundation::PropertyType::BooleanArray:
        case winrt::Windows::Foundation::PropertyType::StringArray:
        case winrt::Windows::Foundation::PropertyType::InspectableArray:
        case winrt::Windows::Foundation::PropertyType::DateTimeArray:
        case winrt::Windows::Foundation::PropertyType::TimeSpanArray:
        case winrt::Windows::Foundation::PropertyType::GuidArray:
            return;
        }

        THROW_HR(E_INVALIDARG);
    }
}
