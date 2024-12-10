// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ServerCommon.h"

#include "../util/TestHelper.h"
#include "Result.h"

using SFS::test::BufferedLogData;
using SFS::test::StatusCodeException;
using SFS::test::details::BaseServerImpl;
using namespace SFS;
using namespace std::string_literals;

static constexpr const char* c_listenHostName = "localhost";

static std::string ToString(httplib::StatusCode status)
{
    return std::to_string(status) + " " + std::string(httplib::status_message(status));
}

StatusCodeException::StatusCodeException(httplib::StatusCode status) : m_status(status), m_message(::ToString(m_status))
{
}

const char* StatusCodeException::what() const noexcept
{
    return m_message.c_str();
}

httplib::StatusCode StatusCodeException::GetStatusCode() const
{
    return m_status;
}

static SFS::LogData ToLogData(const BufferedLogData& data)
{
    return {LogSeverity::Info, data.message.c_str(), data.file.c_str(), data.line, data.function.c_str(), data.time};
}

void BaseServerImpl::Start()
{
    ConfigureServerSettings();
    ConfigureRequestHandlers();

    m_port = m_server.bind_to_any_port(c_listenHostName);
    m_listenerThread = std::thread([&]() { m_server.listen_after_bind(); });
}

void BaseServerImpl::ConfigureServerSettings()
{
    m_server.set_logger([&](const httplib::Request& req, const httplib::Response& res) {
        BUFFER_LOG("Request: " + req.method + " " + req.path + " " + req.version);
        BUFFER_LOG("Request Body: " + req.body);

        BUFFER_LOG("Response: " + res.version + " " + ::ToString(static_cast<httplib::StatusCode>(res.status)) + " " +
                   res.reason);
        BUFFER_LOG("Response body: " + res.body);
    });

    m_server.set_exception_handler([&](const httplib::Request&, httplib::Response& res, std::exception_ptr ep) {
        try
        {
            std::rethrow_exception(ep);
        }
        catch (std::exception& e)
        {
            m_lastException = Result(Result::HttpUnexpected, e.what());
        }
        catch (...)
        {
            m_lastException = Result(Result::HttpUnexpected, "Unknown Exception");
        }

        ProcessBufferedLogs();

        res.status = httplib::StatusCode::InternalServerError_500;
    });

    // Keeping this interval to a minimum ensures tests run quicker
    m_server.set_keep_alive_timeout(1); // 1 second
}

void BaseServerImpl::BufferLog(const BufferedLogData& data)
{
    std::lock_guard guard(m_logMutex);
    m_bufferedLog.push_back(data);
}

BufferedLogData BaseServerImpl::BuildBufferedLogData(const std::string& message,
                                                     const char* file,
                                                     unsigned line,
                                                     const char* function)
{
    return BufferedLogData{GetLogIdentifier() + ": " + message, file, line, function, std::chrono::system_clock::now()};
}

void BaseServerImpl::ProcessBufferedLogs()
{
    for (const auto& data : m_bufferedLog)
    {
        LogCallbackToTest(ToLogData(data));
    }
    m_bufferedLog.clear();
}

Result BaseServerImpl::Stop()
{
    if (m_listenerThread.joinable())
    {
        m_server.stop();
        m_listenerThread.join();
    }
    ProcessBufferedLogs();
    return m_lastException.value_or(Result::Success);
}

std::string BaseServerImpl::GetUrl() const
{
    return "http://"s + c_listenHostName + ":"s + std::to_string(m_port);
}
