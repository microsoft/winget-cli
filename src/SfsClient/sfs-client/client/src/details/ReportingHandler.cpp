// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ReportingHandler.h"

#include <chrono>

using namespace SFS;
using namespace SFS::details;

void ReportingHandler::SetLoggingCallback(LoggingCallbackFn&& callback)
{
    std::lock_guard guard(m_loggingCallbackFnMutex);
    m_loggingCallbackFn = std::move(callback);
}

void ReportingHandler::CallLoggingCallback(LogSeverity severity,
                                           const char* message,
                                           const char* file,
                                           unsigned line,
                                           const char* function) const
{
    std::lock_guard guard(m_loggingCallbackFnMutex);
    if (m_loggingCallbackFn)
    {
        m_loggingCallbackFn({severity, message, file, line, function, std::chrono::system_clock::now()});
    }
}
