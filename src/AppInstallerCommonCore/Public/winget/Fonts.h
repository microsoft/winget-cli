// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <vector>

namespace AppInstaller::Fonts
{
    struct FontFace
    {
        std::wstring FaceName;
        std::vector<std::wstring> FilePaths;
    };

    struct FontFamily
    {
        std::wstring FamilyName;
        std::vector<FontFace> FontFaces;
    };

    /// <summary>
    /// Gets all installed font families on the system.
    /// </summary>
    /// <returns>A list of installed font family names.</returns>
    std::vector<FontFamily> GetInstalledFontFamilies();

    /// <summary>
    /// Gets the installed font family from the provided family name.
    /// </summary>
    /// <param name="familyName"></param>
    /// <returns></returns>
    FontFamily GetInstalledFontFamily(const std::wstring& familyName);
}
