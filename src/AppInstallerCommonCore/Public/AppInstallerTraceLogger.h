// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include <AppInstallerTelemetry.h>

#include <string>
#include <string_view>

namespace AppInstaller::Logging
{
    // Log ETW events for tracing.
    // Doesn't save events to a file on disk.
    struct TraceLogger : ILogger
    {
        TraceLogger() = default;

        ~TraceLogger() = default;

        // ILogger
        virtual std::string GetName() const override;
        virtual void Write(Channel channel, Level, std::string_view message) noexcept override;
    };
}
