// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Logging.h"

#include <mutex>
#include <stdio.h>

#define MAX_LOG_MESSAGE_SIZE 1024

#define LOG_INFO(handler, format, ...)                                                                                 \
    handler.LogWithSeverity(SFS::LogSeverity::Info, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_WARNING(handler, format, ...)                                                                              \
    handler.LogWithSeverity(SFS::LogSeverity::Warning, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_ERROR(handler, format, ...)                                                                                \
    handler.LogWithSeverity(SFS::LogSeverity::Error, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_VERBOSE(handler, format, ...)                                                                              \
    handler.LogWithSeverity(SFS::LogSeverity::Verbose, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

namespace SFS::details
{
/**
 * @brief This class enables thread-safe access to the externally set logging callback function.
 * @details Each SFSClient instance will have one ReportingHandler instance, and access to the logging callback function
 * is controlled by a mutex that makes sure that only one thread can access the logging callback function at a time.
 */
class ReportingHandler
{
  public:
    ReportingHandler() = default;

    ReportingHandler(const ReportingHandler&) = delete;
    ReportingHandler& operator=(const ReportingHandler&) = delete;

    /**
     * @brief Sets the logging callback function.
     * @details This function is thread-safe.
     * @param callback The logging callback function. To reset, pass a nullptr.
     */
    void SetLoggingCallback(LoggingCallbackFn&& callback);

    /**
     * @brief Logs a message with the given severity.
     * @details Prefer calling it with macros LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_VERBOSE so file, line and function
     * are automatically populated and the message can be formatted.
     */
    template <typename... Args>
    void LogWithSeverity(LogSeverity severity,
                         const char* file,
                         unsigned line,
                         const char* function,
                         const char* format,
                         const Args&... args) const
    {
        constexpr std::size_t n = sizeof...(Args);
        if constexpr (n == 0)
        {
            CallLoggingCallback(severity, format, file, line, function);
        }
        else
        {
            char message[MAX_LOG_MESSAGE_SIZE];
            snprintf(message, MAX_LOG_MESSAGE_SIZE, format, args...);
            CallLoggingCallback(severity, message, file, line, function);
        }
    }

  private:
    void CallLoggingCallback(LogSeverity severity,
                             const char* message,
                             const char* file,
                             unsigned line,
                             const char* function) const;

    LoggingCallbackFn m_loggingCallbackFn;
    mutable std::mutex m_loggingCallbackFnMutex;
};
} // namespace SFS::details
