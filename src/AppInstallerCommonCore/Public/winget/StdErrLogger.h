// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>

namespace AppInstaller::Logging
{
    // Sends logs to the stderr stream.
    struct StdErrLogger : ILogger
    {
        StdErrLogger() = default;

        ~StdErrLogger() = default;

        // ILogger
        std::string GetName() const override;

        void Write(Channel channel, Level level, std::string_view message) noexcept override;

        void WriteDirect(Channel channel, Level level, std::string_view message) noexcept override;

        // Adds OutputDebugStringLogger to the current Log
        static void Add();

        // Removes OutputDebugStringLogger from the current Log
        static void Remove();

    private:
        Level m_level = Level::Error;
    };
}
