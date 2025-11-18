// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerFileLogger.h"

#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerDateTime.h"
#include "Public/winget/UserSettings.h"
#include <winget/Filesystem.h>
#include <corecrt_io.h>


namespace AppInstaller::Logging
{
    using namespace std::string_view_literals;
    using namespace std::chrono_literals;

    namespace
    {
        static constexpr std::string_view s_fileLoggerDefaultFilePrefix = "WinGet"sv;
        static constexpr std::string_view s_fileLoggerDefaultFileExt = ".log"sv;

        // Send to a string first to create a single block to write to a file.
        std::string ToLogLine(Channel channel, std::string_view message)
        {
            std::stringstream strstr;
            strstr << std::chrono::system_clock::now() << " [" << std::setw(GetMaxChannelNameLength()) << std::left << std::setfill(' ') << GetChannelName(channel) << "] " << message;
            return std::move(strstr).str();
        }

        // Determines the difference between the given position and the maximum as an offset.
        std::ofstream::off_type CalculateDiff(const std::ofstream::pos_type& position, std::ofstream::off_type maximum)
        {
            auto offsetPosition = static_cast<std::ofstream::off_type>(position);
            return maximum > offsetPosition ? maximum - offsetPosition : 0;
        }
    }

    FileLogger::FileLogger() : FileLogger(s_fileLoggerDefaultFilePrefix) {}

    FileLogger::FileLogger(const std::filesystem::path& filePath)
    {
        m_name = GetNameForPath(filePath);
        m_filePath = filePath;
        GetDefaultMaximumFileSize();
        OpenFileLoggerStream();
    }

    FileLogger::FileLogger(const std::string_view fileNamePrefix)
    {
        m_name = "file";
        m_filePath = Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation);
        m_filePath /= fileNamePrefix.data() + ('-' + Utility::GetCurrentTimeForFilename() + s_fileLoggerDefaultFileExt.data());
        GetDefaultMaximumFileSize();
        OpenFileLoggerStream();
    }

    FileLogger::~FileLogger()
    {
        m_stream.flush();
        // When std::ofstream is constructed from an existing File handle, it does not call fclose on destruction
        // Only calling close() explicitly will close the file handle.
        m_stream.close();
    }

    FileLogger& FileLogger::SetMaximumSize(std::ofstream::off_type maximumSize)
    {
        THROW_HR_IF(E_INVALIDARG, maximumSize < 0);
        m_maximumSize = maximumSize;
        return *this;
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

    void FileLogger::Write(Channel channel, Level level, std::string_view message) noexcept try
    {
        std::string log = ToLogLine(channel, message);
        WriteDirect(channel, level, log);
    }
    catch (...) {}

    void FileLogger::WriteDirect(Channel, Level, std::string_view message) noexcept try
    {
        HandleMaximumFileSize(message);
        m_stream << message << std::endl;
    }
    catch (...) {}

    void FileLogger::SetTag(Tag tag) noexcept try
    {
        if (tag == Tag::HeadersComplete)
        {
            auto currentPosition = m_stream.tellp();
            if (currentPosition != std::ofstream::pos_type{ -1 })
            {
                m_headersEnd = currentPosition;
            }
        }
    }
    catch (...) {}

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
                    const auto& settings = Settings::User();

                    Filesystem::FileLimits fileLimits;
                    fileLimits.Age = settings.Get<Settings::Setting::LoggingFileAgeLimitInDays>();
                    fileLimits.TotalSizeInMB = settings.Get<Settings::Setting::LoggingFileTotalSizeLimitInMB>();
                    fileLimits.Count = settings.Get<Settings::Setting::LoggingFileCountLimit>();

                    auto filesInPath = Filesystem::GetFileInfoFor(filePath);
                    Filesystem::FilterToFilesExceedingLimits(filesInPath, fileLimits);

                    for (const auto& file : filesInPath)
                    {
                        std::filesystem::remove(file.Path);
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

    void FileLogger::GetDefaultMaximumFileSize()
    {
        m_maximumSize = static_cast<std::ofstream::off_type>(Settings::User().Get<Settings::Setting::LoggingFileIndividualSizeLimitInMB>()) << 20;
    }

    void FileLogger::HandleMaximumFileSize(std::string_view& currentLog)
    {
        if (m_maximumSize == 0)
        {
            return;
        }

        auto maximumLogSize = static_cast<size_t>(CalculateDiff(m_headersEnd, m_maximumSize));

        // In the event that a single log is larger than the maximum
        if (currentLog.size() > maximumLogSize)
        {
            currentLog = currentLog.substr(0, maximumLogSize);
            WrapLogFile();
            return;
        }

        auto currentPosition = m_stream.tellp();
        if (currentPosition == std::ofstream::pos_type{ -1 })
        {
            // The expectation is that if the stream is in an error state the write won't actually happen.
            return;
        }

        auto availableSpace = static_cast<size_t>(CalculateDiff(currentPosition, m_maximumSize));

        if (currentLog.size() > availableSpace)
        {
            WrapLogFile();
            return;
        }
    }

    void FileLogger::WrapLogFile()
    {
        m_stream.seekp(m_headersEnd);
        // Yes, we may go over the size limit slightly due to this and the unaccounted for newlines
        m_stream << ToLogLine(Channel::Core, "--- log file has wrapped ---") << std::endl;
    }
}
