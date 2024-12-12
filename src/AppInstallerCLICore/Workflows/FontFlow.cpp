// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontFlow.h"
#include "TableOutput.h"
#include <winget/Fonts.h>
#include <AppInstallerRuntime.h>
#include <FontInstaller.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Font;

    namespace
    {
        struct InstalledFontFamiliesTableLine
        {
            InstalledFontFamiliesTableLine(Utility::LocIndString familyName, int faceCount)
                : FamilyName(familyName), FaceCount(faceCount) {}

            Utility::LocIndString FamilyName;
            int FaceCount;
        };

        struct InstalledFontFacesTableLine
        {
            InstalledFontFacesTableLine(Utility::LocIndString familyName, Utility::LocIndString faceName, Utility::LocIndString faceVersion, std::filesystem::path filePath)
                : FamilyName(familyName), FaceName(faceName), FaceVersion(faceVersion), FilePath(filePath) {}

            Utility::LocIndString FamilyName;
            Utility::LocIndString FaceName;
            Utility::LocIndString FaceVersion;
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
        try
        {
            const auto& installerPath = context.Get<Execution::Data::InstallerPath>();
            std::vector<std::filesystem::path> filePaths;

            // InstallerPath will point to a directory if extracted from an archive.
            if (std::filesystem::is_directory(installerPath))
            {
                const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles = context.Get<Execution::Data::Installer>()->NestedInstallerFiles;
                for (const auto& nestedInstallerFile : nestedInstallerFiles)
                {
                    filePaths.emplace_back(installerPath / ConvertToUTF16(nestedInstallerFile.RelativeFilePath));
                }
            }
            else
            {
                filePaths.emplace_back(installerPath);
            }

            std::vector<FontFile> fontFiles;
            Fonts::FontCatalog fontCatalog;

            for (const auto& file : filePaths)
            {
                DWRITE_FONT_FILE_TYPE fileType;
                if (!fontCatalog.IsFontFileSupported(file, fileType))
                {
                    AICLI_LOG(CLI, Warning, << "Font file is not supported: " << file);
                    context.Reporter.Error() << Resource::String::FontFileNotSupported << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
                }
                else
                {
                    AICLI_LOG(CLI, Verbose, << "Font file is supported: " << file);
                    fontFiles.emplace_back(FontFile(file, fileType));
                }
            }

            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            // TODO: Default to installing to HKEY_LOCAL_MACHINE registry as user install is not yet fully supported. 
            FontInstaller fontInstaller = FontInstaller(Manifest::ScopeEnum::Machine);

            fontInstaller.Install(fontFiles);
            context.Add<Execution::Data::OperationReturnCode>(S_OK);
        }
        catch (...)
        {
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));
        }
    }
}
