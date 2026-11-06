// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestArchive.h"
#include <AppInstallerRuntime.h>
#include <winget/Archive.h>
#include <winget/ManifestYamlParser.h>

namespace AppInstaller::CLI::Workflow
{
    Manifest::Manifest LoadManifestArchive(const std::filesystem::path& archivePath)
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE,
            std::filesystem::file_size(archivePath) == 0 || !Archive::ScanZipFile(archivePath));
        auto extracted = Runtime::GetNewTempFilePath();
        THROW_HR_IF(E_UNEXPECTED, !std::filesystem::create_directory(extracted));
        auto cleanup = wil::scope_exit([&]
        {
            std::error_code error;
            std::filesystem::remove_all(extracted, error);
        });
        THROW_IF_FAILED(Archive::TryExtractArchive(archivePath, extracted));
        std::filesystem::path manifestPath;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(extracted))
        {
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, entry.is_symlink());
            const auto fileExtension = Utility::ToLower(entry.path().extension().u8string());
            if (!entry.is_regular_file() || (fileExtension != ".yaml" && fileExtension != ".yml"))
            {
                continue;
            }
            auto yaml = YAML::Load(entry.path());
            auto type = yaml["ManifestType"].try_as<std::string>().value_or("singleton");
            if (type == "singleton" || type == "version")
            {
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, !manifestPath.empty());
                manifestPath = type == "version" ? entry.path().parent_path() : entry.path();
            }
        }
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING, manifestPath.empty());
        return Manifest::YamlParser::CreateFromPath(manifestPath);
    }
}
