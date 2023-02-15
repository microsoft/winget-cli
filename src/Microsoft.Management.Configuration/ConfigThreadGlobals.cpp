// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigThreadGlobals.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    AppInstaller::Logging::DiagnosticLogger& ConfigThreadGlobals::GetDiagnosticLogger()
    {
        return m_logger;
    }

    AppInstaller::Logging::TelemetryTraceLogger& ConfigThreadGlobals::GetTelemetryLogger()
    {
        THROW_HR(E_NOTIMPL);
    }
}
