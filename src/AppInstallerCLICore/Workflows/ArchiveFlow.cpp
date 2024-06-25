// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ArchiveFlow.h"
#include "PortableFlow.h"
#include "ShellExecuteInstallerHandler.h"
#include <winget/AdminSettings.h>
#include <winget/Archive.h>
#include <winget/Filesystem.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::wstring_view s_Extracted = L"extracted";
    }

    void ScanArchiveFromLocalManifest(Execution::Context& context)
    {
        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            bool scanResult = Archive::ScanZipFile(context.Get<Execution::Data::InstallerPath>());

            if (scanResult)
            {
                AICLI_LOG(CLI, Info, << "Archive malware scan passed");
            }
            else
            {
                if (context.Args.Contains(Execution::Args::Type::IgnoreLocalArchiveMalwareScan) &&
                    Settings::IsAdminSettingEnabled(Settings::BoolAdminSetting::LocalArchiveMalwareScanOverride))
                {
                    AICLI_LOG(CLI, Warning, << "Archive scan detected malware. Proceeding due to --ignore-local-archive-malware-scan");
                    context.Reporter.Warn() << Resource::String::ArchiveFailedMalwareScanOverridden << std::endl;
                }
                else
                {
                    AICLI_LOG(CLI, Error, << "Archive malware scan failed");
                    context.Reporter.Error() << Resource::String::ArchiveFailedMalwareScan << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_ARCHIVE_SCAN_FAILED);
                }
            }
        }
    }

    void ExtractFilesFromArchive(Execution::Context& context)
    {
        const auto& installerPath = context.Get<Execution::Data::InstallerPath>();
        std::filesystem::path destinationFolder = installerPath.parent_path() / s_Extracted;
        std::filesystem::create_directory(destinationFolder);

        AICLI_LOG(CLI, Info, << "Extracting archive to: " << destinationFolder);
        context.Reporter.Info() << Resource::String::ExtractingArchive << std::endl;

        if (Settings::User().Get<Settings::Setting::ArchiveExtractionMethod>() == Archive::ExtractionMethod::Tar)
        {
            context << ShellExecuteExtractArchive(installerPath, destinationFolder);
        }
        else
        {
            HRESULT result = AppInstaller::Archive::TryExtractArchive(installerPath, destinationFolder);

            if (SUCCEEDED(result))
            {
                AICLI_LOG(CLI, Info, << "Successfully extracted archive");
                context.Reporter.Info() << Resource::String::ExtractArchiveSucceeded << std::endl;
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Failed to extract archive with code " << result);
                context.Reporter.Error() << Resource::String::ExtractArchiveFailed << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_EXTRACT_ARCHIVE_FAILED);
            }
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

        std::filesystem::path targetInstallerPath = context.Get<Execution::Data::InstallerPath>().parent_path() / s_Extracted;

        for (const auto& nestedInstallerFile : installer.NestedInstallerFiles)
        {
            const std::filesystem::path& nestedInstallerPath = targetInstallerPath / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);
            
            if (Filesystem::PathEscapesBaseDirectory(nestedInstallerPath, targetInstallerPath))
            {
                AICLI_LOG(CLI, Error, << "Path points to a location outside of the install directory: " << nestedInstallerPath);
                context.Reporter.Error() << Resource::String::InvalidPathToNestedInstaller << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_INVALID_PATH);
            }
            else if (!std::filesystem::exists(nestedInstallerPath))
            {
                AICLI_LOG(CLI, Error, << "Unable to locate nested installer at: " << nestedInstallerPath);
                context.Reporter.Error()
                    << Resource::String::NestedInstallerNotFound(Utility::LocIndView{ nestedInstallerPath.u8string() })
                    << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_NOT_FOUND);
            }
            else if (!IsPortableType(installer.NestedInstallerType))
            {
                // Update the installerPath to the extracted non-portable installer. 
                AICLI_LOG(CLI, Info, << "Setting installerPath to: " << nestedInstallerPath);
                targetInstallerPath = nestedInstallerPath;
            }
        }

        context.Add<Execution::Data::InstallerPath>(targetInstallerPath);
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
