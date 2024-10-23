// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontFlow.h"
#include "TableOutput.h"
#include <winget/Fonts.h>
#include <AppInstallerRuntime.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;

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
            Execution::TableOutput<4> table(context.Reporter, { Resource::String::FontFamily, Resource::String::FontFaces, Resource::String::FontFamily, Resource::String::FontFaces });

            for (size_t i = 0; i < lines.size(); i += 2)
            {
                // Displays 2 font families per line for better readability.
                if ((i + 1) < lines.size())
                {
                    table.OutputLine({ lines[i].FamilyName, std::to_string(lines[i].FaceCount), lines[i+1].FamilyName, std::to_string(lines[i+1].FaceCount) });
                }
                else
                {
                    table.OutputLine({ lines[i].FamilyName, std::to_string(lines[i].FaceCount), {}, {} });
                }
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
        if (context.Args.Contains(Args::Type::Family))
        {
            // TODO: Create custom source and search mechanism for fonts.
            const auto& familyNameArg = context.Args.GetArg(Args::Type::Family);
            const auto& fontFamily = AppInstaller::Fonts::GetInstalledFontFamily(AppInstaller::Utility::ConvertToUTF16(familyNameArg));

            if (!fontFamily.has_value())
            {
                context.Reporter.Info() << Resource::String::NoInstalledFontFound << std::endl;
                return;
            }

            const auto& familyName = Utility::LocIndString(Utility::ConvertToUTF8(fontFamily->Name));
            std::vector<InstalledFontFacesTableLine> lines;

            for (const auto& fontFace : fontFamily->Faces)
            {
                for (const auto& filePath : fontFace.FilePaths)
                {
                    InstalledFontFacesTableLine line(
                        familyName,
                        Utility::LocIndString(Utility::ToLower(Utility::ConvertToUTF8(fontFace.Name))),
                        Utility::LocIndString(Utility::ConvertToUTF8(fontFace.Version)),
                        filePath.u8string());

                    lines.push_back(std::move(line));
                }
            }

            OutputInstalledFontFacesTable(context, lines);
        }
        else
        {
            const auto& fontFamilies = AppInstaller::Fonts::GetInstalledFontFamilies();
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
}
