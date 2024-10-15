// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontFlow.h"
#include <TableOutput.h>
#include <winget/Fonts.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;

    struct InstalledFontFamiliesTableLine
    {
        InstalledFontFamiliesTableLine(Utility::LocIndString familyName, int faceCount)
            : FamilyName(familyName), FaceCount(faceCount) {}

        Utility::LocIndString FamilyName;
        int FaceCount;
    };

    struct InstalledFontFacesTableLine
    {
        InstalledFontFacesTableLine(Utility::LocIndString faceName, Utility::LocIndString familyName, std::filesystem::path filePath)
            : FaceName(faceName), FamilyName(familyName), FilePath(filePath){}

        Utility::LocIndString FaceName;
        Utility::LocIndString FamilyName;
        std::filesystem::path FilePath;
    };

    void OutputInstalledFontFamiliesTable(Execution::Context& context, const std::vector<InstalledFontFamiliesTableLine>& lines)
    {
        Execution::TableOutput<2> table(context.Reporter,
            {
                Resource::String::FontFamilyName,
                Resource::String::FontFaceCount,
            });

        for (const auto& line : lines)
        {
            table.OutputLine({
                line.FamilyName,
                std::to_string(line.FaceCount)
                });
        }
        table.Complete();
    }

    void OutputInstalledFontFacesTable(Execution::Context& context, const std::vector<InstalledFontFacesTableLine>& lines)
    {
        Execution::TableOutput<3> table(context.Reporter,
            {
                Resource::String::FontFaceName,
                Resource::String::FontFamilyName,
                Resource::String::FontFilePaths,
            });

        for (const auto& line : lines)
        {
            table.OutputLine({
                line.FaceName,
                line.FamilyName,
                line.FilePath.u8string(),
                });
        }
        table.Complete();
    }

    void ReportInstalledFontFamiliesResult(Execution::Context& context)
    {
        if (context.Args.Contains(Args::Type::FamilyName))
        {
            const auto& familyNameArg = context.Args.GetArg(Args::Type::FamilyName);
            const auto& fontFamily = AppInstaller::Fonts::GetInstalledFontFamily(AppInstaller::Utility::ConvertToUTF16(familyNameArg));

            std::vector<InstalledFontFacesTableLine> lines;
            const auto& familyName = Utility::LocIndString(familyNameArg);

            for (const auto& fontFace : fontFamily.FontFaces)
            {
                InstalledFontFacesTableLine line(
                    Utility::LocIndString(Utility::ConvertToUTF8(fontFace.FaceName)),
                    familyName,
                    fontFace.FilePaths[0] // Todo: update so that all paths are joined together by new line.
                );

                lines.push_back(std::move(line));
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
                    Utility::LocIndString(Utility::ConvertToUTF8(fontFamily.FamilyName)),
                    (int)fontFamily.FontFaces.size()
                );

                lines.push_back(std::move(line));
            }

            OutputInstalledFontFamiliesTable(context, lines);
        }
    }
}
