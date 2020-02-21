// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace AppInstaller::Logging
{
    // Logs to a file.
    struct FileLogger : public ILogger
    {
        FileLogger(const std::filesystem::path& filePath = {});

        ~FileLogger();

        FileLogger(const FileLogger&) = delete;
        FileLogger& operator=(const FileLogger&) = delete;

        FileLogger(FileLogger&&) = default;
        FileLogger& operator=(FileLogger&&) = default;

        static std::string GetNameForPath(const std::filesystem::path& filePath);

        // ILogger
        virtual std::string GetName() const override;

        virtual void Write(Channel channel, Level level, std::string_view message) noexcept override;

    private:
        std::string m_name;
        std::filesystem::path m_filePath;
        std::ofstream m_stream;
    };
}
