// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace AppInstaller::Logging
{
    // Logs to a file.
    struct FileLogger : public ILogger
    {
        FileLogger();
        explicit FileLogger(const std::filesystem::path& filePath);
        explicit FileLogger(const std::string_view fileNamePrefix);

        ~FileLogger();

        FileLogger(const FileLogger&) = delete;
        FileLogger& operator=(const FileLogger&) = delete;

        FileLogger(FileLogger&&) = default;
        FileLogger& operator=(FileLogger&&) = default;

        static std::string GetNameForPath(const std::filesystem::path& filePath);

        static std::string_view DefaultPrefix();
        static std::string_view DefaultExt();

        // ILogger
        std::string GetName() const override;

        void Write(Channel channel, Level level, std::string_view message) noexcept override;

        void WriteDirect(Channel channel, Level level, std::string_view message) noexcept override;

        // Adds a FileLogger to the current Log
        static void Add();
        static void Add(const std::filesystem::path& filePath);
        static void Add(std::string_view fileNamePrefix);

        // Starts a background task to clean up old log files.
        static void BeginCleanup();
        static void BeginCleanup(const std::filesystem::path& filePath);

    private:
        std::string m_name;
        std::filesystem::path m_filePath;
        std::ofstream m_stream;

        void OpenFileLoggerStream();
    };
}
