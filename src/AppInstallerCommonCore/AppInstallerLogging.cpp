// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerLogging.h"

#include "Public/AppInstallerFileLogger.h"
#include "Public/AppInstallerTelemetry.h"
#include "Public/AppInstallerDateTime.h"
#include "Public/AppInstallerRuntime.h"

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

    char const* const GetChannelName(Channel channel)
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

    DiagnosticLogger& DiagnosticLogger::GetInstance()
    {
        static DiagnosticLogger instance;
        return instance;
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

        if (IsEnabled(channel, level))
        {
            for (auto& logger : m_loggers)
            {
                logger->Write(channel, level, message);
            }
        }
    }

    void AddFileLogger(const std::filesystem::path& filePath)
    {
        Log().AddLogger(std::make_unique<FileLogger>(filePath));
    }

    void BeginLogFileCleanup()
    {
        FileLogger::BeginCleanup(Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation));
    }
}

std::ostream& operator<<(std::ostream& out, const std::chrono::system_clock::time_point& time)
{
    AppInstaller::Utility::OutputTimepoint(out, time);
    return out;
}
