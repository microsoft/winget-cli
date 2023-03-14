// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Helper to enable objects to enforce being immutable.
    struct MutableFlag
    {
        MutableFlag(bool isMutable = true) : m_isMutable(isMutable) {}
        MutableFlag(const MutableFlag&) = default;
        MutableFlag& operator=(const MutableFlag&) = default;
        MutableFlag(MutableFlag&&) = default;
        MutableFlag& operator=(MutableFlag&&) = default;

        // If the state is not mutable, throws.
        void RequireMutable() const;

    private:
        bool m_isMutable;
    };
}
