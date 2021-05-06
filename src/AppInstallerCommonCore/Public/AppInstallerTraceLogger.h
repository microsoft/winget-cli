// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include <AppInstallerTelemetry.h>

#include <string>
#include <string_view>

namespace AppInstaller::Logging
{
    struct TraceLogger : ILogger
    {
        TraceLogger() = default;

        ~TraceLogger() = default;

        virtual std::string GetName() const override;

        virtual void Write(Channel channel, Level, std::string_view message) noexcept override;
    };
}