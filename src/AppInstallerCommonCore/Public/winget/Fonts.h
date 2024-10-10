// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <vector>

namespace AppInstaller::Fonts
{
    /// <summary>
    /// Gets all installed font families on the system.
    /// </summary>
    /// <returns>A list of installed font family names.</returns>
    std::vector<std::wstring> GetInstalledFontFamilies();
}
