// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string_view>

namespace AppInstaller::CLI
{
    class TelemetryTraceLogger
    {
    public:
        static TelemetryTraceLogger& GetInstance();
        void LogMessage(std::wstring_view message);

    private:
        TelemetryTraceLogger();
        ~TelemetryTraceLogger();
    };
}