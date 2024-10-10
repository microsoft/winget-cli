// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontFlow.h"
#include <TableOutput.h>
#include <winget/Fonts.h>

namespace AppInstaller::CLI::Workflow
{
    struct InstalledFontFamiliesTableLine
    {
        InstalledFontFamiliesTableLine(Utility::LocIndString familyName, int faceCount)
            : FamilyName(familyName), FaceCount(faceCount) {}

        Utility::LocIndString FamilyName;
        int FaceCount;
    };

    void OutputInstalledFontFamiliesTable(Execution::Context& context, const std::vector<InstalledFontFamiliesTableLine>& lines)
    {
        Execution::TableOutput<2> table(context.Reporter,
            {
                Resource::String::SearchName,
                Resource::String::SearchId,
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

    void ReportInstalledFontFamiliesResult(Execution::Context& context)
    {
        const auto& fontFamilyNames = AppInstaller::Fonts::GetInstalledFontFamilies();

        std::vector<InstalledFontFamiliesTableLine> lines;

        for (const auto& familyName : fontFamilyNames)
        {
            InstalledFontFamiliesTableLine line(
                Utility::LocIndString(Utility::ConvertToUTF8(familyName)),
                1
            );

            lines.push_back(std::move(line));
        }

        OutputInstalledFontFamiliesTable(context, lines);
    }
}
