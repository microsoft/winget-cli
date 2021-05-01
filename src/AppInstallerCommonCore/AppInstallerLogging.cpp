// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerLogging.h"

#include "Public/AppInstallerFileLogger.h"
#include "Public/AppInstallerTelemetry.h"
#include "Public/AppInstallerDateTime.h"
#include "Public/AppInstallerRuntime.h"

thread_local AppInstaller::Logging::ThreadGlobals* t_pThreadGlobals = nullptr;

namespace AppInstaller::Logging
{
    namespace
    {
        template <typename E>
        std::underlying_type_t<E> AsNum(E e)
        {
            return static_cast<std::underlying_type_t<E>>(e);
        }

        uint64_t ConvertChannelToBitmask(Channel channel)
        {
            if (channel == Channel::All)
            {
                return std::numeric_limits<uint64_t>::max();
            }
            else
            {
                return (1ull << AsNum(channel));
            }
        }
    }

    char const* GetChannelName(Channel channel)
    {
        switch(channel)
        {
        case Channel::Fail: return "FAIL";
        case Channel::CLI:  return "CLI";
        case Channel::SQL:  return "SQL";
        case Channel::Repo: return "REPO";
        case Channel::YAML: return "YAML";
        case Channel::Core: return "CORE";
        case Channel::Test: return "TEST";
        default:            return "NONE";
        }
    }

    size_t GetMaxChannelNameLength() { return 4; }

    void TraceLogger::LogMessage(std::string str) noexcept try
    {
        TraceLoggingWriteActivity(g_hTraceProvider,
            "Diagnostics",
            t_pThreadGlobals->GetTelemetryLogger().GetActivityId(),
            nullptr,
            TraceLoggingString(str.c_str(), "LogMessage"));
    }
    catch (...)
    {
        // Just eat any exceptions here; better to lose logs than functionality
    }

    void DiagnosticLogger::AddLogger(std::unique_ptr<ILogger>&& logger)
    {
        m_loggers.emplace_back(std::move(logger));
    }

    bool DiagnosticLogger::ContainsLogger(const std::string& name)
    {
        for (auto i = m_loggers.begin(); i != m_loggers.end(); ++i)
        {
            if ((*i)->GetName() == name)
            {
                return true;
            }
        }

        return false;
    }

    std::unique_ptr<ILogger> DiagnosticLogger::RemoveLogger(const std::string& name)
    {
        std::unique_ptr<ILogger> result;

        for (auto i = m_loggers.begin(); i != m_loggers.end(); ++i)
        {
            if ((*i)->GetName() == name)
            {
                result = std::move(*i);
                m_loggers.erase(i);
                break;
            }
        }

        return result;
    }

    void DiagnosticLogger::RemoveAllLoggers()
    {
        m_loggers.clear();
    }

    void DiagnosticLogger::EnableChannel(Channel channel)
    {
        m_enabledChannels |= ConvertChannelToBitmask(channel);
    }

    void DiagnosticLogger::DisableChannel(Channel channel)
    {
        m_enabledChannels &= ~ConvertChannelToBitmask(channel);
    }

    void DiagnosticLogger::SetLevel(Level level)
    {
        m_enabledLevel = level;
    }

    bool DiagnosticLogger::IsEnabled(Channel channel, Level level) const
    {
        return (!m_loggers.empty() &&
                (m_enabledChannels & ConvertChannelToBitmask(channel)) != 0 &&
                (AsNum(level) >= AsNum(m_enabledLevel)));
    }

    void DiagnosticLogger::Write(Channel channel, Level level, std::string_view message)
    {
        THROW_HR_IF_MSG(E_INVALIDARG, channel == Channel::All, "Cannot write to all channels");

        // Send to a string first to create a single block to write to a file.
        std::stringstream strstr;
        strstr << std::chrono::system_clock::now() << " [" << std::setw(GetMaxChannelNameLength()) << std::left << std::setfill(' ') << GetChannelName(channel) << "] " << message;

        if (IsEnabled(channel, level))
        {
            for (auto& logger : m_loggers)
            {
                logger->Write(strstr.str());
            }
        }
        traceLogger.LogMessage(strstr.str());
    }

    void AddFileLogger(const std::filesystem::path& filePath)
    {
        Log().AddLogger(std::make_unique<FileLogger>(filePath));
    }

    void BeginLogFileCleanup()
    {
        FileLogger::BeginCleanup(Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation));
    }

    std::ostream& SetHRFormat(std::ostream& out)
    {
        return out << std::hex << std::setw(8) << std::setfill('0');
    }

    DiagnosticLogger& ThreadGlobals::GetDiagnosticLogger()
    {
        if (!m_pDiagnosticLogger)
        {
            try
            {
                std::call_once(diagLoggerInitOnceFlag, [this]()
                {
                    InitDiagnosticLogger();
                });
            }
            catch (...)
            {
                // May throw std::system_error if any condition prevents calls to call_once from executing as specified
                // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
            }
        }
        return *(m_pDiagnosticLogger.get());
    }

    void ThreadGlobals::InitDiagnosticLogger()
    {
        try
        {
            m_pDiagnosticLogger = std::make_unique<DiagnosticLogger>();
        }
        catch (...)
        {
            // May throw std::bad_alloc or any exception thrown by the constructor of DiagnosticLogger
            // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
        }
    }

    TelemetryTraceLogger& ThreadGlobals::GetTelemetryLogger()
    {
        if (!m_pTelemetryLogger)
        {
            try
            {
                std::call_once(telLoggerInitOnceFlag, [this]()
                {
                    InitTelemetryLogger();
                });
            }
            catch (...)
            {
                // May throw std::system_error if any condition prevents calls to call_once from executing as specified
                // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
            }
        }
        return *(m_pTelemetryLogger.get());
    }

    void ThreadGlobals::InitTelemetryLogger()
    {
        try
        {
            m_pTelemetryLogger = std::make_unique<TelemetryTraceLogger>();
        }
        catch (...)
        {
            // May throw std::bad_alloc or any exception thrown by the constructor of TelemetryTraceLogger
            // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
        }
    }
}

std::ostream& operator<<(std::ostream& out, const std::chrono::system_clock::time_point& time)
{
    AppInstaller::Utility::OutputTimePoint(out, time);
    return out;
}
