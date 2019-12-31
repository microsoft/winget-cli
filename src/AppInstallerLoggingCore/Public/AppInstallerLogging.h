// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::Logging
{
    // The channel that the log is from.
    // Channels enable large groups of logs to be enabled or disabled together.
    enum class Channel : uint32_t
    {
        CLI,
        SQL,
        Repo,
        YAML,
        All,
    };

    // The level of the log.
    enum class Level
    {
        Verbose,
        Info,
        Error,
        Crit,
    };

    // The interface that a log target must implement.
    struct ILogger
    {
        virtual ~ILogger() = default;

        // Gets the name of the logger for internal use.
        virtual std::string GetName() const = 0;

        // Informs the logger of the given log.
        virtual void Write(Channel channel, Level level, std::wstring_view message) = 0;
    };

    // This type contains the set of loggers that diagnostic logging will be sent to.
    // Each binary that leverages it must configure any loggers and filters to their
    // desired level, as nothing is enabled by default.
    struct DiagnosticLogger
    {
        // Gets the singleton instance of this type.
        static DiagnosticLogger& GetInstance();

        // Adds a logger to the active set.
        void AddLogger(std::unique_ptr<ILogger>&& logger);

        // Removes a logger from the active set, returning it.
        std::unique_ptr<ILogger> RemoveLogger(const std::string& name);

        // Enables the given channel.
        void EnableChannel(Channel channel);

        // Disables the given channel.
        void DisableChannel(Channel channel);

        // Sets the enabled level.
        // All levels higher than this level will be enabled.
        // For example; SetLevel(Verbose) will enable all logs.
        void SetLevel(Level level);

        // Checks whether a given channel and level are enabled.
        bool IsEnabled(Channel channel, Level level) const;

        // Writes a log line, if the given channel and level are enabled.
        void Write(Channel channel, Level level, std::wstring_view message);

    private:
        DiagnosticLogger() = default;
        ~DiagnosticLogger() = default;

        std::vector<std::unique_ptr<ILogger>> _loggers;
    };

    // Helper to make the call sites look clean.
    inline DiagnosticLogger& Log()
    {
        return DiagnosticLogger::GetInstance();
    }
}
