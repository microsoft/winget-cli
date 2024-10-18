// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontFlow.h"
#include "TableOutput.h"
#include <winget/Fonts.h>

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
            InstalledFontFacesTableLine(Utility::LocIndString faceName, Utility::LocIndString familyName, std::filesystem::path filePath)
                : FaceName(faceName), FamilyName(familyName), FilePath(filePath) {}

            Utility::LocIndString FaceName;
            Utility::LocIndString FamilyName;
            std::filesystem::path FilePath;
        };

        void OutputInstalledFontFamiliesTable(Execution::Context& context, const std::vector<InstalledFontFamiliesTableLine>& lines)
        {
            Execution::TableOutput<2> table(context.Reporter, { Resource::String::FontFamily, Resource::String::FontFaceCount });

            for (const auto& line : lines)
            {
                table.OutputLine({ line.FamilyName, std::to_string(line.FaceCount) });
            }

            table.Complete();
        }

        void OutputInstalledFontFacesTable(Execution::Context& context, const std::vector<InstalledFontFacesTableLine>& lines)
        {
            Execution::TableOutput<3> table(context.Reporter, { Resource::String::FontFace, Resource::String::FontFamily, Resource::String::FontFilePaths });

            for (const auto& line : lines)
            {
                table.OutputLine({ line.FaceName, line.FamilyName, line.FilePath.u8string() });
            }

            table.Complete();
        }
    }

    void ReportInstalledFonts(Execution::Context& context)
    {
        if (context.Args.Contains(Args::Type::Family))
        {
            // TODO: Utilize font index for better searching capability.
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
                bool isFirstLine = true;
                for (const auto& filePath : fontFace.FilePaths)
                {
                    if (isFirstLine)
                    {
                        InstalledFontFacesTableLine line(
                            Utility::LocIndString(Utility::ToLower(Utility::ConvertToUTF8(fontFace.Name))),
                            familyName,
                            filePath.u8string()
                        );
                        isFirstLine = false;
                        lines.push_back(std::move(line));
                    }
                    else
                    {
                        InstalledFontFacesTableLine line({}, {}, filePath.u8string());
                        lines.push_back(std::move(line));
                    }
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
