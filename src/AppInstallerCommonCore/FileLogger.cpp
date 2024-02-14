// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerFileLogger.h"

#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerDateTime.h"
#include <corecrt_io.h>


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
        OpenFileLoggerStream();
    }

    FileLogger::FileLogger(const std::string_view fileNamePrefix)
    {
        m_name = "file";
        m_filePath = Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation);
        m_filePath /= fileNamePrefix.data() + ('-' + Utility::GetCurrentTimeForFilename() + s_fileLoggerDefaultFileExt.data());
        OpenFileLoggerStream();
    }

    FileLogger::~FileLogger()
    {
        m_stream.flush();
        // When std::ofstream is constructed from an existing File handle, it does not call fclose on destruction
        // Only calling close() explicitly will close the file handle.
        m_stream.close();
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

    void FileLogger::WriteDirect(Channel, Level, std::string_view message) noexcept try
    {
        m_stream << message << std::endl;
    }
    catch (...)
    {
        // Just eat any exceptions here; better than losing logs
    }

    void FileLogger::Add()
    {
        Log().AddLogger(std::make_unique<FileLogger>());
    }

    void FileLogger::Add(const std::filesystem::path& filePath)
    {
        Log().AddLogger(std::make_unique<FileLogger>(filePath));
    }

    void FileLogger::Add(std::string_view fileNamePrefix)
    {
        Log().AddLogger(std::make_unique<FileLogger>(fileNamePrefix));
    }

    void FileLogger::BeginCleanup()
    {
        BeginCleanup(Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation));
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

    void FileLogger::OpenFileLoggerStream() 
    {
        // Prevent other writers to our log file, but allow readers
        FILE* filePtr = _wfsopen(m_filePath.wstring().c_str(), L"w", _SH_DENYWR);

        if (filePtr)
        {
            auto closeFile = wil::scope_exit([&]() { fclose(filePtr); });

            // Prevent inheritance to ensure log file handle is not opened by other processes
            THROW_IF_WIN32_BOOL_FALSE(SetHandleInformation(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(filePtr))), HANDLE_FLAG_INHERIT, 0));

            m_stream = std::ofstream{ filePtr };
            closeFile.release();
        }
        else
        {
            AICLI_LOG(Core, Error, << "Failed to open log file " << m_filePath.u8string());
            throw std::system_error(errno, std::generic_category());
        }
    }
}
