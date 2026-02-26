// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct DiagnosticInformationInstance : winrt::implements<DiagnosticInformationInstance, IDiagnosticInformation>
    {
        DiagnosticInformationInstance() = default;

        void Initialize(DiagnosticLevel level, std::wstring_view message);

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
