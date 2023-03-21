// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DiagnosticInformation.h"
#include "DiagnosticInformation.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void DiagnosticInformation::Initialize(DiagnosticLevel level, std::wstring_view message)
    {
        m_level = level;
        m_message = message;
    }

    DiagnosticLevel DiagnosticInformation::Level()
    {
        return m_level;
    }

    void DiagnosticInformation::Level(DiagnosticLevel value)
    {
        m_level = value;
    }

    hstring DiagnosticInformation::Message()
    {
        return m_message;
    }

    void DiagnosticInformation::Message(const hstring& value)
    {
        m_message = value;
    }
}
