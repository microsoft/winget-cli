// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerFileLogger.h"

#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerDateTime.h"


namespace AppInstaller::Logging
{
    using namespace std::string_view_literals;
    using namespace std::chrono_literals;

    static constexpr std::string_view s_fileLoggerDefaultFilePrefix = "WinGet"sv;
    static constexpr std::string_view s_fileLoggerDefaultFileExt = ".log"sv;

    FileLogger::FileLogger() : FileLogger(s_fileLoggerDefaultFilePrefix) {}

    FileLogger::FileLogger(const std::filesystem::path& filePath)
    {
        m_name = GetNameForPath(filePath);
        m_filePath = filePath;

        m_stream.open(m_filePath);
    }

    FileLogger::FileLogger(const std::string_view fileNamePrefix)
    {
        m_name = "file";
        m_filePath = Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation);
        m_filePath /= fileNamePrefix.data() + ('-' + Utility::GetCurrentTimeForFilename() + s_fileLoggerDefaultFileExt.data());

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

    std::string_view FileLogger::DefaultPrefix()
    {
        return s_fileLoggerDefaultFilePrefix;
    }

    std::string_view FileLogger::DefaultExt()
    {
        return s_fileLoggerDefaultFileExt;
    }

    std::string FileLogger::GetName() const
    {
        return m_name;
    }

    void FileLogger::Write(Channel channel, Level, std::string_view message) noexcept try
    {
        // Send to a string first to create a single block to write to a file.
        std::stringstream strstr;
        strstr << std::chrono::system_clock::now() << " [" << std::setw(GetMaxChannelNameLength()) << std::left << std::setfill(' ') << GetChannelName(channel) << "] " << message;
        m_stream << strstr.str() << std::endl;
    }
    catch (...)
    {
        // Just eat any exceptions here; better than losing logs
    }

    void FileLogger::BeginCleanup(const std::filesystem::path& filePath)
    {
        std::thread([filePath]()
            {
                try
                {
                    auto now = std::filesystem::file_time_type::clock::now();

                    // Remove all files that are older than 7 days from the standard log location.
                    for (auto& file : std::filesystem::directory_iterator{ filePath })
                    {
                        if (file.is_regular_file() &&
                            now - file.last_write_time() > (7 * 24h))
                        {
                            std::filesystem::remove(file.path());
                        }
                    }
                }
                // Just throw out everything
                catch (...) {}
            }).detach();
    }
}
