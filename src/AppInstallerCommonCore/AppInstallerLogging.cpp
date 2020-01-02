// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerLogging.h"

#include "FileLogger.h"

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
        case Channel::CLI:  return "CLI";
        case Channel::SQL:  return "SQL";
        case Channel::Repo: return "REPO";
        case Channel::YAML: return "YAML";
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
        return ((m_enabledChannels & ConvertChannelToBitmask(channel)) != 0 &&
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

    void AddDefaultFileLogger()
    {
        Log().AddLogger(std::make_unique<FileLogger>());
    }
}
