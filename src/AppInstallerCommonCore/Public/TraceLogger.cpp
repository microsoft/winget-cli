// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerTraceLogger.h"
#include "Public/ThreadGlobals.h"

namespace AppInstaller::Logging
{
    void TraceLogger::Write(Channel channel, Level, std::string_view message) noexcept try
    {
        // Send to a string first to create a single block to write to a file.
        std::stringstream strstr;
        strstr << std::chrono::system_clock::now() << " [" << std::setw(GetMaxChannelNameLength()) << std::left << std::setfill(' ') << GetChannelName(channel) << "] " << message << std::endl;

        using namespace AppInstaller::ThreadLocalStorage;

        TraceLoggingWriteActivity(g_hTraceProvider,
            "Diagnostics",
            ThreadGlobals::GetForCurrentThread()->GetTelemetryLogger().GetActivityId(),
            nullptr,
            TraceLoggingString(strstr.str().c_str(), "LogMessage"));
    }
    catch (...)
    {
        // Just eat any exceptions here; better to lose logs than functionality
    }

    std::string TraceLogger::GetName() const
    {
        return "TraceLogger";
    }
}
