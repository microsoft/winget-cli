// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>

namespace AppInstaller::Logging
{
    // Sends logs to the OutputDebugString function.
    // Intended for use during initialization debugging.
    struct OutputDebugStringLogger : ILogger
    {
        OutputDebugStringLogger() = default;

        ~OutputDebugStringLogger() = default;

        // ILogger
        std::string GetName() const override;

        void Write(Channel channel, Level, std::string_view message) noexcept override;

        void WriteDirect(Channel channel, Level level, std::string_view message) noexcept override;

        // Adds OutputDebugStringLogger to the current Log
        static void Add();

        // Removes OutputDebugStringLogger from the current Log
        static void Remove();
    };
}
