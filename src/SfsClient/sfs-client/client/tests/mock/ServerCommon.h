// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Logging.h"
#include "Result.h"

#include <httplib.h>

#include <chrono>
#include <optional>
#include <string>

namespace SFS::test
{
class StatusCodeException : public std::exception
{
  public:
    StatusCodeException(httplib::StatusCode status);

    const char* what() const noexcept override;

    httplib::StatusCode GetStatusCode() const;

  private:
    httplib::StatusCode m_status;
    std::string m_message;
};

#define BUILD_BUFFERED_LOG_DATA(message) BuildBufferedLogData(message, __FILE__, __LINE__, __FUNCTION__)

#define BUFFER_LOG(message) BufferLog(BUILD_BUFFERED_LOG_DATA(message))

struct BufferedLogData
{
    std::string message;
    std::string file;
    unsigned line;
    std::string function;
    std::chrono::time_point<std::chrono::system_clock> time;
};

namespace details
{
class BaseServerImpl
{
  public:
    BaseServerImpl() = default;
    ~BaseServerImpl() = default;

    BaseServerImpl(const BaseServerImpl&) = delete;
    BaseServerImpl& operator=(const BaseServerImpl&) = delete;

    void Start();
    Result Stop();

    std::string GetUrl() const;

  protected:
    void ConfigureServerSettings();
    virtual void ConfigureRequestHandlers() = 0;

    virtual std::string GetLogIdentifier() = 0;

    void BufferLog(const BufferedLogData& data);
    BufferedLogData BuildBufferedLogData(const std::string& message,
                                         const char* file,
                                         unsigned line,
                                         const char* function);
    void ProcessBufferedLogs();

    httplib::Server m_server;
    int m_port{-1};

    std::optional<Result> m_lastException;

    std::thread m_listenerThread;

    std::vector<BufferedLogData> m_bufferedLog;
    std::mutex m_logMutex;
};
} // namespace details
} // namespace SFS::test
