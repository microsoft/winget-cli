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
        void Level(DiagnosticLevel value);

        hstring Message();
        void Message(const hstring& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        DiagnosticLevel m_level = DiagnosticLevel::Verbose;
        hstring m_message;
#endif
    };
}

namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct DiagnosticInformation : DiagnosticInformationT<DiagnosticInformation, implementation::DiagnosticInformation>
    {
    };
}
