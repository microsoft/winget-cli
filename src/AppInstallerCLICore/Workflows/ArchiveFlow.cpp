// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ArchiveFlow.h"
#include "winget/Archive.h"
#include "winget/Filesystem.h"
#include "PortableFlow.h"
#include "PortableInstaller.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    void ExtractFilesFromArchive(Execution::Context& context)
    {
        const auto& installerPath = context.Get<Execution::Data::InstallerPath>();
        std::filesystem::path destinationFolder;
        std::vector<std::filesystem::path> extractedItems;
        bool isDirectoryCreated = false;

        HRESULT hr;
        if (context.Get<Execution::Data::Installer>()->NestedInstallerType == InstallerTypeEnum::Portable)
        {
            destinationFolder = GetPortableTargetDirectory(context);
            isDirectoryCreated = std::filesystem::create_directory(destinationFolder);
            hr = AppInstaller::Archive::TryExtractArchive(installerPath, destinationFolder, extractedItems);
        }
        else
        {
            destinationFolder = installerPath.parent_path();
            hr = AppInstaller::Archive::TryExtractArchive(installerPath, destinationFolder, extractedItems);
        }

        AICLI_LOG(CLI, Info, << "Extracting archive to: " << destinationFolder);

        if (SUCCEEDED(hr))
        {
            AICLI_LOG(CLI, Info, << "Successfully extracted archive");
            context.Add<Execution::Data::ExtractedItems>(extractedItems);
        }
        else
        {
            if (isDirectoryCreated)
            {
                std::filesystem::remove(destinationFolder);
            }

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


        InstallerTypeEnum nestedInstallerType = installer.NestedInstallerType;
        std::filesystem::path destinationFolder;
        if (nestedInstallerType == InstallerTypeEnum::Portable)
        {
            destinationFolder = GetPortableTargetDirectory(context);
        }
        else
        {
            destinationFolder = context.Get<Execution::Data::InstallerPath>().parent_path();
        }

        for (const auto& nestedInstallerFile : installer.NestedInstallerFiles)
        {
            const std::filesystem::path& nestedInstallerPath = destinationFolder / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);
            
            if (Filesystem::PathEscapesBaseDirectory(nestedInstallerPath, destinationFolder))
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
            else if (nestedInstallerType != InstallerTypeEnum::Portable)
            {
                // Only update the installerPath if it points to a non-portable installer.
                AICLI_LOG(CLI, Info, << "Setting installerPath to: " << nestedInstallerPath);
                context.Add<Execution::Data::InstallerPath>(nestedInstallerPath);
            }
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