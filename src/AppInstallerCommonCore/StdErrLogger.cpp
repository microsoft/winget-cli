// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/StdErrLogger.h"

namespace AppInstaller::Logging
{
    namespace
    {
        static constexpr std::string_view s_StdErrLoggerName = "StdErrLogger";
    }

    std::string StdErrLogger::GetName() const
    {
        return std::string{ s_StdErrLoggerName };
    }

    void StdErrLogger::Write(Channel channel, Level level, std::string_view message) noexcept try
    {
        if (level >= m_level)
        {
            std::cerr << "[" << std::setw(GetMaxChannelNameLength()) << std::left << std::setfill(' ') << GetChannelName(channel) << "] " << message << std::endl;
        }
    }
    catch (...)
    {
        // Just eat any exceptions here; better than losing logs
    }

    void StdErrLogger::WriteDirect(Channel, Level level, std::string_view message) noexcept try
    {
        if (level >= m_level)
        {
            std::cerr.write(message.data(), static_cast<std::streamsize>(message.size()));
        }
    }
    catch (...)
    {
        // Just eat any exceptions here; better than losing logs
    }

    void StdErrLogger::Add()
    {
        Log().AddLogger(std::make_unique<StdErrLogger>());
    }

    void StdErrLogger::Remove()
    {
        Log().RemoveLogger(std::string{ s_StdErrLoggerName });
    }
}
