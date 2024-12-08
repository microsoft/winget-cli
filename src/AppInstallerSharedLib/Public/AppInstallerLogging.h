// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <chrono>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#define AICLI_LOG_DIRECT(_logger_,_channel_,_level_,_outstream_) \
    do { \
        auto _aicli_log_channel = AppInstaller::Logging::Channel:: _channel_; \
        auto _aicli_log_level = AppInstaller::Logging::Level:: _level_; \
        auto& _aicli_log_log = _logger_; \
        if (_aicli_log_log.IsEnabled(_aicli_log_channel, _aicli_log_level)) \
        { \
            AppInstaller::Logging::LoggingStream _aicli_log_strstr; \
            _aicli_log_strstr _outstream_; \
            _aicli_log_log.Write(_aicli_log_channel, _aicli_log_level, _aicli_log_strstr.str()); \
        } \
    } while (0, 0)

#define AICLI_LOG(_channel_,_level_,_outstream_) AICLI_LOG_DIRECT(AppInstaller::Logging::Log(),_channel_,_level_,_outstream_)

// Consider using this macro when the string might be larger than 4K.
// The normal macro has some buffering that occurs; it can cut off larger strings and is slower.
#define AICLI_LOG_LARGE_STRING(_channel_,_level_,_headerStream_,_largeString_) \
    do { \
        auto _aicli_log_channel = AppInstaller::Logging::Channel:: _channel_; \
        auto _aicli_log_level = AppInstaller::Logging::Level:: _level_; \
        auto& _aicli_log_log = AppInstaller::Logging::Log(); \
        if (_aicli_log_log.IsEnabled(_aicli_log_channel, _aicli_log_level)) \
        { \
            AppInstaller::Logging::LoggingStream _aicli_log_strstr; \
            _aicli_log_strstr _headerStream_; \
            _aicli_log_log.Write(_aicli_log_channel, _aicli_log_level, _aicli_log_strstr.str()); \
            _aicli_log_log.WriteDirect(_aicli_log_channel, _aicli_log_level, _largeString_); \
        } \
    } while (0, 0)

namespace AppInstaller::Logging
{
    // The channel that the log is from.
    // Channels enable large groups of logs to be enabled or disabled together.
    enum class Channel : uint32_t
    {
        Fail = 0x1,
        CLI = 0x2,
        SQL = 0x4,
        Repo = 0x8,
        YAML = 0x10,
        Core = 0x20,
        Test = 0x40,
        Config = 0x80,
        Workflow = 0x100,
        None = 0,
        All = 0xFFFFFFFF,
        Defaults = All & ~(SQL | Workflow),
    };

    DEFINE_ENUM_FLAG_OPERATORS(Channel);

    // Gets the channel's name as a string.
    std::string_view GetChannelName(Channel channel);

    // Gets the channel from it's name.
    Channel GetChannelFromName(std::string_view channel);

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

        // Informs the logger of the given log with the intention that no buffering occurs (in winget code).
        virtual void WriteDirect(Channel channel, Level level, std::string_view message) noexcept = 0;
    };

    // This type contains the set of loggers that diagnostic logging will be sent to.
    // Each binary that leverages it must configure any loggers and filters to their
    // desired level, as nothing is enabled by default.
    struct DiagnosticLogger
    {
        DiagnosticLogger() = default;

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
        // All levels above this level will be enabled.
        // For example; SetLevel(Verbose) will enable all logs.
        void SetLevel(Level level);

        // Gets the enabled level.
        Level GetLevel() const;

        // Checks whether a given channel and level are enabled.
        bool IsEnabled(Channel channel, Level level) const;

        // Writes a log line, if the given channel and level are enabled.
        void Write(Channel channel, Level level, std::string_view message);

        // Writes a log line, if the given channel and level are enabled.
        // Use to make large logs more efficient by writing directly to the output streams.
        void WriteDirect(Channel channel, Level level, std::string_view message);

    private:

        std::vector<std::unique_ptr<ILogger>> m_loggers;
        Channel m_enabledChannels = Channel::None;
        Level m_enabledLevel = Level::Info;
    };

    DiagnosticLogger& Log();

    // Calls the various stream format functions to produce an 8 character hexadecimal output.
    std::ostream& SetHRFormat(std::ostream& out);

    // This type allows us to override the default behavior of output operators for logging.
    struct LoggingStream
    {
        // Force use of the UTF-8 string from a file path.
        // This should not be necessary when we move to C++20 and convert to using u8string.
        friend AppInstaller::Logging::LoggingStream& operator<<(AppInstaller::Logging::LoggingStream& out, const std::filesystem::path& path)
        {
            out.m_out << path.u8string();
            return out;
        }

        // Enums
        template <typename T>
        friend std::enable_if_t<std::is_enum_v<std::decay_t<T>>, AppInstaller::Logging::LoggingStream&>
            operator<<(AppInstaller::Logging::LoggingStream& out, T t)
        {
            out.m_out << ToIntegral(t);
            return out;
        }

        // Everything else.
        template <typename T>
        friend std::enable_if_t<!std::disjunction_v<std::is_same<std::decay_t<T>, std::filesystem::path>, std::is_enum<std::decay_t<T>>>, AppInstaller::Logging::LoggingStream&>
            operator<<(AppInstaller::Logging::LoggingStream& out, T&& t)
        {
            out.m_out << std::forward<T>(t);
            return out;
        }

        std::string str() const { return m_out.str(); }

    private:
        std::stringstream m_out;
    };
}

namespace std
{
    std::ostream& operator<<(std::ostream& out, const std::chrono::system_clock::time_point& time);
    std::ostream& operator<<(std::ostream& out, const GUID& guid);
}
