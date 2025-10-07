// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "FontFlow.h"
#include "TableOutput.h"
#include <winget/Fonts.h>
#include <AppInstallerRuntime.h>
using namespace AppInstaller::Utility;

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::Fonts;

    namespace
    {
        struct InstalledFontFamiliesTableLine
        {
            InstalledFontFamiliesTableLine(Utility::LocIndString familyName, int faceCount)
                : FamilyName(familyName), FaceCount(faceCount) {
            }

            Utility::LocIndString FamilyName;
            int FaceCount;
        };

        struct InstalledFontFacesTableLine
        {
            InstalledFontFacesTableLine(Utility::LocIndString familyName, Utility::LocIndString faceName, Utility::LocIndString faceVersion, std::filesystem::path filePath)
                : FamilyName(familyName), FaceName(faceName), FaceVersion(faceVersion), FilePath(filePath) {
            }

            Utility::LocIndString FamilyName;
            Utility::LocIndString FaceName;
            Utility::LocIndString FaceVersion;
            std::filesystem::path FilePath;
        };

        struct InstalledFontFilesTableLine
        {
            InstalledFontFilesTableLine(Utility::LocIndString title, Utility::LocIndString packageName, Resource::LocString fontStatus, std::filesystem::path filePath)
                : Title(title), PackageName(packageName), FontStatus(fontStatus), FilePath(filePath) {
            }

            Utility::LocIndString Title;
            Utility::LocIndString PackageName;
            Resource::LocString FontStatus;
            std::filesystem::path FilePath;
        };

        void OutputInstalledFontFamiliesTable(Execution::Context& context, const std::vector<InstalledFontFamiliesTableLine>& lines)
        {
            Execution::TableOutput<2> table(context.Reporter, { Resource::String::FontFamily, Resource::String::FontFaces });

            for (auto line : lines)
            {
                table.OutputLine({ line.FamilyName, std::to_string(line.FaceCount) });
            }

            table.Complete();
        }

        void OutputInstalledFontFacesTable(Execution::Context& context, const std::vector<InstalledFontFacesTableLine>& lines)
        {
            Execution::TableOutput<4> table(context.Reporter, { Resource::String::FontFamily, Resource::String::FontFace, Resource::String::FontVersion, Resource::String::FontFilePaths });

            bool anonymizePath = Settings::User().Get<Settings::Setting::AnonymizePathForDisplay>();

            for (auto line : lines)
            {
                if (anonymizePath)
                {
                    AppInstaller::Runtime::ReplaceProfilePathsWithEnvironmentVariable(line.FilePath);
                }

                table.OutputLine({ line.FamilyName, line.FaceName, line.FaceVersion, line.FilePath.u8string() });
            }

            table.Complete();
        }

        void OutputInstalledFontFilesTable(Execution::Context& context, const std::vector<InstalledFontFilesTableLine>& lines)
        {
            Execution::TableOutput<4> table(context.Reporter, { Resource::String::FontTitle, Resource::String::FontPackage, Resource::String::FontStatus, Resource::String::FontFilePaths });

            bool anonymizePath = Settings::User().Get<Settings::Setting::AnonymizePathForDisplay>();

            for (auto line : lines)
            {
                if (anonymizePath)
                {
                    AppInstaller::Runtime::ReplaceProfilePathsWithEnvironmentVariable(line.FilePath);
                }

                table.OutputLine({ line.Title, line.PackageName, line.FontStatus, line.FilePath.u8string() });
            }

            table.Complete();
        }
    }

    void ReportInstalledFonts(Execution::Context& context)
    {
        Fonts::FontCatalog fontCatalog;

        if (context.Args.Contains(Args::Type::Family))
        {
            // TODO: Create custom source and search mechanism for fonts.
            const auto& familyNameArg = AppInstaller::Utility::ConvertToUTF16(context.Args.GetArg(Args::Type::Family));
            const auto& fontFamilies = fontCatalog.GetInstalledFontFamilies(familyNameArg);

            if (fontFamilies.empty())
            {
                context.Reporter.Info() << Resource::String::NoInstalledFontFound << std::endl;
                return;
            }

            std::vector<InstalledFontFacesTableLine> lines;

            for (const auto& fontFamily : fontFamilies)
            {
                const auto& familyName = Utility::LocIndString(Utility::ConvertToUTF8(fontFamily.Name));

                for (const auto& fontFace : fontFamily.Faces)
                {
                    for (const auto& filePath : fontFace.FilePaths)
                    {
                        InstalledFontFacesTableLine line(
                            familyName,
                            Utility::LocIndString(Utility::ToLower(Utility::ConvertToUTF8(fontFace.Name))),
                            Utility::LocIndString(fontFace.Version.ToString()),
                            filePath.u8string());

                        lines.push_back(std::move(line));
                    }
                }
            }

            OutputInstalledFontFacesTable(context, lines);
        }
        else if (context.Args.Contains(Args::Type::Details))
        {
            const auto& fontFiles = AppInstaller::Fonts::GetInstalledFontFiles();
            std::vector<InstalledFontFilesTableLine> lines;
            for (const auto& fontFile : fontFiles)
            {
                Resource::LocString status;
                switch (fontFile.Status)
                {
                case FontStatus::OK:
                    status = Resource::LocString(Resource::String::FontStatusOK);
                    break;
                case FontStatus::Corrupt:
                    status = Resource::LocString(Resource::String::FontStatusCorrupt);
                    break;
                default:
                    status = Resource::LocString(Resource::String::FontStatusUnknown);
                    break;
                }

                InstalledFontFilesTableLine line(
                    Utility::LocIndString(Utility::ConvertToUTF8(fontFile.Title)),
                    Utility::LocIndString(Utility::ConvertToUTF8(fontFile.PackageIdentifier)),
                    status,
                    fontFile.FilePath.u8string());

                lines.push_back(std::move(line));
            }

            OutputInstalledFontFilesTable(context, lines);
        }
        else
        {
            const auto& fontFamilies = fontCatalog.GetInstalledFontFamilies();
            std::vector<InstalledFontFamiliesTableLine> lines;

            for (const auto& fontFamily : fontFamilies)
            {
                InstalledFontFamiliesTableLine line(
                    Utility::LocIndString(Utility::ConvertToUTF8(fontFamily.Name)),
                    static_cast<int>(fontFamily.Faces.size())
                );

                lines.push_back(std::move(line));
            }

            OutputInstalledFontFamiliesTable(context, lines);
        }
    }

    void FontInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        // We will default to User scope.
        Manifest::ScopeEnum scope = Manifest::ScopeEnum::User;
        if (context.Args.Contains(Execution::Args::Type::InstallScope))
        {
            scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        }

        Fonts::FontContext fontContext;
        fontContext.InstallerSource = InstallerSource::WinGet;
        fontContext.Scope = scope;

        auto& manifest = context.Get<Execution::Data::Manifest>();
        fontContext.PackageId = ConvertToUTF16(manifest.Id);
        fontContext.PackageVersion = ConvertToUTF16(manifest.Version);

        if (context.Args.Contains(Execution::Args::Type::Force))
        {
            fontContext.Force = true;
        }

        try
        {
            const auto& installerPath = context.Get<Execution::Data::InstallerPath>();

            // InstallerPath will point to a directory if extracted from an archive.
            if (std::filesystem::is_directory(installerPath))
            {
                const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles = context.Get<Execution::Data::Installer>()->NestedInstallerFiles;
                for (const auto& nestedInstallerFile : nestedInstallerFiles)
                {
                    fontContext.AddPackageFile(installerPath / ConvertToUTF16(nestedInstallerFile.RelativeFilePath));
                }
            }
            else
            {
                fontContext.AddPackageFile(installerPath);
            }

            const auto& fontValidationResult = Fonts::ValidateFontPackage(fontContext);
            if (fontValidationResult.Result != FontResult::Success)
            {
                context.Reporter.Error() << Resource::String::FontValidationFailed << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED);
            }

            if (fontValidationResult.HasUnsupportedFonts)
            {
                context.Reporter.Error() << Resource::String::FontFileNotSupported << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
            }

            if (fontValidationResult.Status == FontStatus::OK && !fontContext.Force)
            {
                context.Reporter.Warn() << Resource::String::FontAlreadyInstalled << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_FONT_ALREADY_INSTALLED);
            }

            auto installResult = Fonts::InstallFontPackage(fontContext);
            if (installResult.Result() != FontResult::Success)
            {
                context.Reporter.Error() << Resource::String::FontInstallFailed << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_FONT_INSTALL_FAILED);
            }

            context.Add<Execution::Data::CorrelatedAppsAndFeaturesEntries>({ fontContext.GetAppsAndFeaturesEntry() });
            context.Add<Execution::Data::OperationReturnCode>(installResult.HResult);
        }
        catch (...)
        {
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));
            context.Reporter.Warn() << Resource::String::FontInstallFailed << std::endl;

            try
            {
                // The Font Install code handles rollback where appropriate. If we hit an
                // unexpected exception, try to uninstall anyway to arrive at a consistent
                // absent state. Since we install side-by-side for versions, this should
                // only result in an absent state of an installed font if this were a forced
                // install of an existing version.
                auto uninstallResult = Fonts::UninstallFontPackage(fontContext);
                if (uninstallResult.Result() != FontResult::Success)
                {
                    context.Reporter.Warn() << Resource::String::FontRollbackFailed << std::endl;
                }
            }
            CATCH_LOG();

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_FONT_INSTALL_FAILED);
        }
    }

    void FontUninstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

        try
        {
            // We will default to installed scope.
            auto scope = Manifest::ConvertToScopeEnum(context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[Repository::PackageVersionMetadata::InstalledScope]);
            if (context.Args.Contains(Execution::Args::Type::InstallScope))
            {
                scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            }

            Fonts::FontContext fontContext;
            fontContext.InstallerSource = InstallerSource::WinGet;
            if (context.Args.Contains(Execution::Args::Type::Manifest))
            {
                const auto& manifest = context.Get<Execution::Data::Manifest>();
                fontContext.PackageId = ConvertToUTF16(manifest.Id);
                fontContext.PackageVersion = ConvertToUTF16(manifest.Version);
                fontContext.Scope = scope;
            }
            else
            {
                const std::string moniker = context.Get<Execution::Data::InstalledPackageVersion>()->GetProperty(AppInstaller::Repository::PackageVersionProperty::Moniker);
                const std::string version = context.Get<Execution::Data::InstalledPackageVersion>()->GetProperty(AppInstaller::Repository::PackageVersionProperty::Version);
                fontContext.PackageId = ConvertToUTF16(moniker);
                fontContext.PackageVersion = ConvertToUTF16(version);
                fontContext.Scope = scope;
            }

            if (fontContext.Scope == Manifest::ScopeEnum::Machine && !Runtime::IsRunningAsAdmin())
            {
                context.Reporter.Error() << Resource::String::CommandRequiresAdmin << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN);
            }

            auto uninstallResult = Fonts::UninstallFontPackage(fontContext);
            if (uninstallResult.Result() != FontResult::Success)
            {
                context.Reporter.Error() << Resource::String::FontUninstallFailed << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED);
            }

            context.Add<Execution::Data::OperationReturnCode>(uninstallResult.HResult);
        }
        catch (...)
        {
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));
            context.Reporter.Error() << Resource::String::FontUninstallFailed << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED);
        }
    }
}
