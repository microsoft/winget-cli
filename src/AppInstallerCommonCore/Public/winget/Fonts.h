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
    };

    struct FontFamily
    {
        std::wstring Name;
        std::vector<FontFace> Faces;
    };

    /// <summary>
    /// Gets all installed font families on the system.
    /// </summary>
    /// <returns>A list of installed font family names.</returns>
    std::vector<FontFamily> GetInstalledFontFamilies();

    /// <summary>
    /// Gets the installed font family from the provided family name.
    /// </summary>
    /// <param name="familyName">The font family name.</param>
    /// <returns>The Font Family.</returns>
    std::optional<FontFamily> GetInstalledFontFamily(const std::wstring& familyName);
}
