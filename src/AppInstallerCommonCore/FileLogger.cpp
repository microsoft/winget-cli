// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FileLogger.h"

#include "Public/AppInstallerRuntime.h"
#include "DateTime.h"

#define AICLI_FILELOGGER_DEFAULT_FILE "AICLI.log"

namespace AppInstaller::Logging
{

    FileLogger::FileLogger(const std::filesystem::path& filePath)
    {
        if (filePath.empty())
        {
            m_name = "file";
            m_filePath = Runtime::GetPathToTemp();
            m_filePath /= AICLI_FILELOGGER_DEFAULT_FILE;
        }
        else
        {
            m_name = GetNameForPath(filePath);
            m_filePath = filePath;
        }

        m_stream.open(m_filePath);
    }

    FileLogger::~FileLogger()
    {
        m_stream.flush();
    }

    std::string FileLogger::GetNameForPath(const std::filesystem::path& filePath)
    {
        using namespace std::string_literals;
        return "file :: "s + filePath.u8string();
    }

    std::string FileLogger::GetName() const
    {
        return m_name;
    }

    void FileLogger::Write(Channel channel, Level, std::string_view message)
    {
        Utility::OutputTimepoint(m_stream, std::chrono::system_clock::now());
        m_stream << " [" << std::setw(GetMaxChannelNameLength()) << std::left << std::setfill(' ') << GetChannelName(channel) << "] " << message << std::endl;
    }
}
