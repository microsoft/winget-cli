// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Logging.h"

using SFS::LogSeverity;

std::string_view SFS::ToString(LogSeverity severity) noexcept
{
    switch (severity)
    {
    case LogSeverity::Info:
        return "Info";
    case LogSeverity::Warning:
        return "Warning";
    case LogSeverity::Error:
        return "Error";
    case LogSeverity::Verbose:
        return "Verbose";
    }
    return "";
}
