// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.Foundation.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Ensures that the given type is supported.
    void EnsureSupportedType(Windows::Foundation::PropertyType type);
}
