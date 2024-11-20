// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/OutputDebugStringLogger.h"

namespace AppInstaller::Logging
{
    namespace
    {
        static constexpr std::string_view s_OutputDebugStringLoggerName = "OutputDebugStringLogger";
    }

    std::string OutputDebugStringLogger::GetName() const
    {
        return std::string{ s_OutputDebugStringLoggerName };
    }

    void OutputDebugStringLogger::Write(Channel channel, Level, std::string_view message) noexcept try
    {
        std::stringstream strstr;
        strstr << "[" << std::setw(GetMaxChannelNameLength()) << std::left << std::setfill(' ') << GetChannelName(channel) << "] " << message << std::endl;
        std::string formattedMessage = std::move(strstr).str();

        OutputDebugStringA(formattedMessage.c_str());
    }
    catch (...)
    {
        // Just eat any exceptions here; better than losing logs
    }

    void OutputDebugStringLogger::WriteDirect(Channel, Level, std::string_view message) noexcept try
    {
        std::string nullTerminatedMessage{ message };
        OutputDebugStringA(nullTerminatedMessage.c_str());
    }
    catch (...)
    {
        // Just eat any exceptions here; better than losing logs
    }

    void OutputDebugStringLogger::Add()
    {
        Log().AddLogger(std::make_unique<OutputDebugStringLogger>());
    }

    void OutputDebugStringLogger::Remove()
    {
        Log().RemoveLogger(std::string{ s_OutputDebugStringLoggerName });
    }
}
