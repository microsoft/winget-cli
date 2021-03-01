// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#define AICLI_LOG(_channel_,_level_,_outstream_) \
    do { \
        auto _aicli_log_channel = AppInstaller::Logging::Channel:: _channel_; \
        auto _aicli_log_level = AppInstaller::Logging::Level:: _level_; \
        auto& _aicli_log_log = AppInstaller::Logging::Log(); \
        if (_aicli_log_log.IsEnabled(_aicli_log_channel, _aicli_log_level)) \
        { \
            AppInstaller::Logging::LoggingStream _aicli_log_strstr; \
            _aicli_log_strstr _outstream_; \
            _aicli_log_log.Write(_aicli_log_channel, _aicli_log_level, _aicli_log_strstr.str()); \
        } \
    } while (0, 0)

namespace AppInstaller::Logging
{
    // The channel that the log is from.
    // Channels enable large groups of logs to be enabled or disabled together.
    enum class Channel : uint32_t
    {
        Fail,
        CLI,
        SQL,
        Repo,
        YAML,
        Core,
        Test,
        All,
    };

    // Gets the channel's name as a string.
    char const* GetChannelName(Channel channel);

    // Gets the maximum channel name length in characters.
    size_t GetMaxChannelNameLength();

    // The level of the log.
    enum class Level
    {
        Verbose,
        Info,
        Warning,
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
        virtual void Write(Channel channel, Level level, std::string_view message) noexcept = 0;
    };

    // This type contains the set of loggers that diagnostic logging will be sent to.
    // Each binary that leverages it must configure any loggers and filters to their
    // desired level, as nothing is enabled by default.
    struct DiagnosticLogger
    {
        ~DiagnosticLogger() = default;

        DiagnosticLogger(const DiagnosticLogger&) = delete;
        DiagnosticLogger& operator=(const DiagnosticLogger&) = delete;

        DiagnosticLogger(DiagnosticLogger&&) = delete;
        DiagnosticLogger& operator=(DiagnosticLogger&&) = delete;

        // Gets the singleton instance of this type.
        static DiagnosticLogger& GetInstance();

        // NOTE: The logger management functionality is *SINGLE THREAD SAFE*.
        //       This includes with logging itself.
        //       As it is not expected that adding/removing loggers is an
        //       extremely frequent operation, no care has been made to protect
        //       it from modifying loggers while logging may be occurring.

        // Adds a logger to the active set.
        void AddLogger(std::unique_ptr<ILogger>&& logger);

        // Determines if a logger with the given name is present.
        bool ContainsLogger(const std::string& name);

        // Removes a logger from the active set, returning it.
        std::unique_ptr<ILogger> RemoveLogger(const std::string& name);

        // Removes all loggers.
        void RemoveAllLoggers();

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
        void Write(Channel channel, Level level, std::string_view message);

    private:
        DiagnosticLogger() = default;

        std::vector<std::unique_ptr<ILogger>> m_loggers;
        uint64_t m_enabledChannels = 0;
        Level m_enabledLevel = Level::Info;
    };

    // Helper to make the call sites look clean.
    inline DiagnosticLogger& Log()
    {
        return DiagnosticLogger::GetInstance();
    }

    // Adds the default file logger to the DiagnosticLogger.
    void AddFileLogger(const std::filesystem::path& filePath = {});

    // Starts a background task to clean up old log files.
    void BeginLogFileCleanup();

    // Calls the various stream format functions to produce an 8 character hexadecimal output.
    std::ostream& SetHRFormat(std::ostream& out);

    // This type allows us to override the default behavior of output operators for logging.
    struct LoggingStream
    {
        // Force use of the UTF-8 string from a file path.
        // This should not be necessary when we move to C++20 and convert to using u8string.
        friend AppInstaller::Logging::LoggingStream& operator<<(AppInstaller::Logging::LoggingStream& out, std::filesystem::path& path)
        {
            out.m_out << path.u8string();
            return out;
        }

        friend AppInstaller::Logging::LoggingStream& operator<<(AppInstaller::Logging::LoggingStream& out, const std::filesystem::path& path)
        {
            out.m_out << path.u8string();
            return out;
        }

        // Everything else.
        template <typename T>
        friend AppInstaller::Logging::LoggingStream& operator<<(AppInstaller::Logging::LoggingStream& out, T&& t)
        {
            out.m_out << std::forward<T>(t);
            return out;
        }

        std::string str() const { return m_out.str(); }

    private:
        std::stringstream m_out;
    };
}

// Enable output of system_clock time_points.
std::ostream& operator<<(std::ostream& out, const std::chrono::system_clock::time_point& time);
