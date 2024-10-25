// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <string>
#include <vector>

namespace AppInstaller::Fonts
{
    struct FontFace
    {
        std::wstring Name;
        std::vector<std::filesystem::path> FilePaths;
        Utility::OpenTypeFontVersion Version;
    };

    struct FontFamily
    {
        std::wstring Name;
        std::vector<FontFace> Faces;
    };

    /// <summary>
    /// Gets all installed font families on the system. If an exact family name is provided and found, returns the installed font family.
    /// </summary>
    /// <returns>A list of installed font families.</returns>
    std::vector<FontFamily> GetInstalledFontFamilies(std::optional<std::wstring> familyName = {});
}
