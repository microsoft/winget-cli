// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <vector>

namespace AppInstaller::Fonts
{
    struct FontFace
    {
        std::wstring Name;
        std::vector<std::filesystem::path> FilePaths;
        std::wstring Version;
    };

    struct FontFamily
    {
        std::wstring Name;
        std::vector<FontFace> Faces;
        std::wstring Version;
    };

    /// <summary>
    /// Gets all installed font families on the system. If an exact family name is provided and found, returns the installed font family.
    /// </summary>
    /// <returns>A list of installed font families.</returns>
    std::vector<FontFamily> GetInstalledFontFamilies(std::optional<std::wstring> familyName = {});
}
