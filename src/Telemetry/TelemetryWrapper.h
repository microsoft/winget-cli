// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "pch.h"
#include "TraceLogging.h"

namespace AppInstaller::CLI
{
    class TelemetryTraceLogger
    {
    public:
        static TelemetryTraceLogger& GetInstance();
        virtual void LogMessage(std::wstring_view message);

    private:
        TelemetryTraceLogger();
        ~TelemetryTraceLogger();
    };
}