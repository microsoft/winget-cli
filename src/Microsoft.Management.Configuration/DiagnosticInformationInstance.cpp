// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DiagnosticInformationInstance.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void DiagnosticInformationInstance::Initialize(DiagnosticLevel level, std::wstring_view message)
    {
        m_level = level;
        m_message = message;
    }

    DiagnosticLevel DiagnosticInformationInstance::Level()
    {
        return m_level;
    }

    void DiagnosticInformationInstance::Level(DiagnosticLevel value)
    {
        m_level = value;
    }

    hstring DiagnosticInformationInstance::Message()
    {
        return m_message;
    }

    void DiagnosticInformationInstance::Message(const hstring& value)
    {
        m_message = value;
    }
}
