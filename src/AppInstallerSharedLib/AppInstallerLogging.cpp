// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerDateTime.h"
#include "Public/winget/SharedThreadGlobals.h"

namespace AppInstaller::Logging
{
    std::string_view GetChannelName(Channel channel)
    {
        switch(channel)
        {
        case Channel::Fail:   return "FAIL";
        case Channel::CLI:    return "CLI";
        case Channel::SQL:    return "SQL";
        case Channel::Repo:   return "REPO";
        case Channel::YAML:   return "YAML";
        case Channel::Core:   return "CORE";
        case Channel::Test:   return "TEST";
        case Channel::Config: return "CONF";
        case Channel::Workflow: return "WORK";
        default:              return "NONE";
        }
    }

    Channel GetChannelFromName(std::string_view channel)
    {
        std::string lowerChannel = Utility::ToLower(channel);

        if (lowerChannel == "fail")
        {
            return Channel::Fail;
        }
        else if (lowerChannel == "cli")
        {
            return Channel::CLI;
        }
        else if (lowerChannel == "sql")
        {
            return Channel::SQL;
        }
        else if (lowerChannel == "repo")
        {
            return Channel::Repo;
        }
        else if (lowerChannel == "yaml")
        {
            return Channel::YAML;
        }
        else if (lowerChannel == "core")
        {
            return Channel::Core;
        }
        else if (lowerChannel == "test")
        {
            return Channel::Test;
        }
        else if (lowerChannel == "conf" || lowerChannel == "config")
        {
            return Channel::Config;
        }
        else if (lowerChannel == "workflow")
        {
            return Channel::Workflow;
        }
        else if (lowerChannel == "default" || lowerChannel == "defaults")
        {
            return Channel::Defaults;
        }
        else if (lowerChannel == "all")
        {
            return Channel::All;
        }

        return Channel::None;
    }

    size_t GetMaxChannelNameLength() { return 4; }

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
        WI_SetAllFlags(m_enabledChannels, channel);
    }

    void DiagnosticLogger::DisableChannel(Channel channel)
    {
        WI_ClearAllFlags(m_enabledChannels, channel);
    }

    void DiagnosticLogger::SetLevel(Level level)
    {
        m_enabledLevel = level;
    }

    Level DiagnosticLogger::GetLevel() const
    {
        return m_enabledLevel;
    }

    bool DiagnosticLogger::IsEnabled(Channel channel, Level level) const
    {
        return (!m_loggers.empty() &&
                WI_IsAnyFlagSet(m_enabledChannels, channel) &&
                (ToIntegral(level) >= ToIntegral(m_enabledLevel)));
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

    void DiagnosticLogger::WriteDirect(Channel channel, Level level, std::string_view message)
    {
        THROW_HR_IF_MSG(E_INVALIDARG, channel == Channel::All, "Cannot write to all channels");

        if (IsEnabled(channel, level))
        {
            for (auto& logger : m_loggers)
            {
                logger->WriteDirect(channel, level, message);
            }
        }
    }

    DiagnosticLogger& Log()
    {
        ThreadLocalStorage::ThreadGlobals* pThreadGlobals = ThreadLocalStorage::ThreadGlobals::GetForCurrentThread();
        if (pThreadGlobals)
        {
            return pThreadGlobals->GetDiagnosticLogger();
        }
        else
        {
            static DiagnosticLogger processGlobalLogger;
            return processGlobalLogger;
        }
    }

    std::ostream& SetHRFormat(std::ostream& out)
    {
        return out << std::hex << std::setw(8) << std::setfill('0');
    }
}

namespace std
{
    std::ostream& operator<<(std::ostream& out, const std::chrono::system_clock::time_point& time)
    {
        AppInstaller::Utility::OutputTimePoint(out, time);
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const GUID& guid)
    {
        wchar_t buffer[256];

        if (StringFromGUID2(guid, buffer, ARRAYSIZE(buffer)))
        {
            out << AppInstaller::Utility::ConvertToUTF8(buffer);
        }
        else
        {
            out << "error";
        }

        return out;
    }
}
