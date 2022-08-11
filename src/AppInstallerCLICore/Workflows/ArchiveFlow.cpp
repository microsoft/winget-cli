// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ArchiveFlow.h"
#include "winget/Archive.h"
#include "winget/Filesystem.h"

using namespace AppInstaller::Manifest;

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
            // Pre-install validation should prevent this from happening
            AICLI_LOG(CLI, Error, << "No entries specified for NestedInstallerFiles");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);
        }

        const auto& installerPath = context.Get<Execution::Data::InstallerPath>();
        const auto& installerParentPath = installerPath.parent_path();
        const auto& relativeFilePath = ConvertToUTF16(installer.NestedInstallerFiles[0].RelativeFilePath);

        const std::filesystem::path& nestedInstallerPath = installerParentPath / relativeFilePath;

        if (Filesystem::PathEscapesBaseDirectory(nestedInstallerPath, installerParentPath))
        {
            AICLI_LOG(CLI, Error, << "Path points to a location outside of the install directory: " << nestedInstallerPath);
            context.Reporter.Error() << Resource::String::InvalidPathToNestedInstaller << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_INVALID_PATH);
        }
        else if (!std::filesystem::exists(nestedInstallerPath))
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

    void EnsureValidNestedInstallerMetadataForArchiveInstall(Execution::Context& context)
    {
        auto installer = context.Get<Execution::Data::Installer>().value();

        if (IsArchiveType(installer.BaseInstallerType))
        {
            if (!IsNestedInstallerTypeSupported(installer.NestedInstallerType))
            {
                AICLI_LOG(CLI, Error, << "Nested installer type not supported: " << installer.NestedInstallerType);
                context.Reporter.Error() << Resource::String::NestedInstallerNotSupported << std::endl;
                AICLI_TERMINATE_CONTEXT(ERROR_NOT_SUPPORTED);
            }

            auto const& nestedInstallerFiles = installer.NestedInstallerFiles;
            if (nestedInstallerFiles.empty())
            {
                AICLI_LOG(CLI, Error, << "No entries specified for NestedInstallerFiles");
                context.Reporter.Error() << Resource::String::NestedInstallerNotSpecified << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);
            }

            if (installer.NestedInstallerType != InstallerTypeEnum::Portable && nestedInstallerFiles.size() != 1)
            {
                AICLI_LOG(CLI, Error, << "Multiple nested installers specified for non-portable nested installerType");
                context.Reporter.Error() << Resource::String::MultipleNonPortableNestedInstallersSpecified << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);
            }
        }
    }
}