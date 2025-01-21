// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.Foundation.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Ensures that the given type is supported.
    void EnsureSupportedType(Windows::Foundation::PropertyType type);

    // Ensures that the value object matches the expected property type.
    bool IsValidObjectType(Windows::Foundation::IInspectable const& value, Windows::Foundation::PropertyType type);

    // Ensures that the value object matches the expected property type.
    void EnsureObjectType(Windows::Foundation::IInspectable const& value, Windows::Foundation::PropertyType type);

    // Determines if the given type supports comparison.
    bool IsComparableType(Windows::Foundation::PropertyType type);

    // Ensures that the given type supports comparison.
    void EnsureComparableType(Windows::Foundation::PropertyType type);

    // Determines if the given type supports length restrictions.
    bool IsLengthType(Windows::Foundation::PropertyType type);

    // Ensures that the given type supports length restrictions.
    void EnsureLengthType(Windows::Foundation::PropertyType type);

    // Determines if the given type is a scalar that can be converted to a reasonable string representation.
    bool IsStringableType(Windows::Foundation::PropertyType type);

    // Gets the string version of the given property, if it is stringable.
    hstring ToString(Windows::Foundation::IPropertyValue value);
}
