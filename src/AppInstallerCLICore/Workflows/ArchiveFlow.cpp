// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ArchiveFlow.h"
#include "winget/Archive.h"

namespace AppInstaller::CLI::Workflow
{
    void ExtractInstallerFromArchive(Execution::Context& context)
    {
        const auto& installerPath = context.Get<Execution::Data::InstallerPath>();
        const auto& installerParentPath = installerPath.parent_path();

        HRESULT hr = AppInstaller::Archive::ExtractArchive(installerPath, installerParentPath);

        if (SUCCEEDED(hr))
        {
            AICLI_LOG(CLI, Info, << "Successfully extracted archive");
            context.SetFlags(Execution::ContextFlag::InstallerExtractedFromArchive);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Failed to extract archive");
            context.Reporter.Error() << Resource::String::ExtractArchiveFailed << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_EXTRACT_ARCHIVE_FAILED);
        }
    }

    void VerifyAndSetNestedInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();
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