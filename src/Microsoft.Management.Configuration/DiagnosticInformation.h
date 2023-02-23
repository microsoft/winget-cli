// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "DiagnosticInformation.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct DiagnosticInformation : DiagnosticInformationT<DiagnosticInformation>
    {
        DiagnosticInformation() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(DiagnosticLevel level, std::wstring_view message);
#endif

        DiagnosticLevel Level();
        hstring Message();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        DiagnosticLevel m_level = DiagnosticLevel::Verbose;
        hstring m_message;
#endif
    };
}
