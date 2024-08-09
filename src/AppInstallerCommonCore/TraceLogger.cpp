// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/TraceLogger.h"
#include "Public/AppInstallerTelemetry.h"
#include "Public/winget/ThreadGlobals.h"

namespace AppInstaller::Logging
{
    void TraceLogger::Write(Channel channel, Level, std::string_view message) noexcept try
    {
        // Send to a string first to create a single block to log to a trace.
        std::stringstream strstr;
        strstr << std::chrono::system_clock::now() << " [" << std::setw(GetMaxChannelNameLength()) << std::left << std::setfill(' ') << GetChannelName(channel) << "] " << message << std::endl;

        TraceLoggingWriteActivity(g_hTraceProvider,
            "Diagnostics",
            Telemetry().GetActivityId(),
            Telemetry().GetParentActivityId(),
            TraceLoggingString(strstr.str().c_str(), "LogMessage"));
    }
    catch (...)
    {
        // Just eat any exceptions here; better to lose logs than functionality
    }

    void TraceLogger::WriteDirect(Channel, Level, std::string_view message) noexcept try
    {
        TraceLoggingWriteActivity(g_hTraceProvider,
            "Diagnostics",
            Telemetry().GetActivityId(),
            Telemetry().GetParentActivityId(),
            TraceLoggingCountedUtf8String(message.data(), static_cast<ULONG>(message.size()), "LogMessage"));
    }
    catch (...)
    {
        // Just eat any exceptions here; better to lose logs than functionality
    }

    std::string TraceLogger::GetName() const
    {
        return "Trace";
    }

    void TraceLogger::Add()
    {
        Log().AddLogger(std::make_unique<TraceLogger>());
    }
}
