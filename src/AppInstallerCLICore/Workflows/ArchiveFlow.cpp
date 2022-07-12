// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ArchiveFlow.h"
#include "winget/Archive.h"

namespace AppInstaller::CLI::Workflow
{
    void ExtractFilesFromArchive(Execution::Context& context)
    {
        const auto& installerPath = context.Get<Execution::Data::InstallerPath>();
        const auto& installerParentPath = installerPath.parent_path();

        // TODO: For portables, extract portables to final install location and log to local database.
        HRESULT hr = AppInstaller::Archive::TryExtractArchive(installerPath, installerParentPath);
        AICLI_LOG(CLI, Info, << "Extracting archive to: " << installerParentPath);

        if (SUCCEEDED(hr))
        {
            AICLI_LOG(CLI, Info, << "Successfully extracted archive");
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Failed to extract archive with code " << hr);
            context.Reporter.Error() << Resource::String::ExtractArchiveFailed << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_EXTRACT_ARCHIVE_FAILED);
        }
    }

    void VerifyAndSetNestedInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();
        if (installer.NestedInstallerFiles.empty())
        {
            AICLI_LOG(CLI, Error, << "No entries specified for NestedInstallerFiles");
            context.Reporter.Error() << Resource::String::NestedInstallerFilesNotSpecified << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);
        }

        const auto& installerPath = context.Get<Execution::Data::InstallerPath>();
        const auto& installerParentPath = installerPath.parent_path();

        const auto& relativeFilePath = ConvertToUTF16(installer.NestedInstallerFiles[0].RelativeFilePath);

        std::filesystem::path nestedInstallerPath = installerParentPath / relativeFilePath;

        if (!std::filesystem::exists(nestedInstallerPath))
        {
            AICLI_LOG(CLI, Error, << "Unable to locate nested installer at: " << nestedInstallerPath);
            context.Reporter.Error() << Resource::String::NestedInstallerNotFound << ' ' << nestedInstallerPath << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_NOT_FOUND);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Setting installerPath to: " << nestedInstallerPath);
            context.Add<Execution::Data::InstallerPath>(nestedInstallerPath);
        }
    }
}