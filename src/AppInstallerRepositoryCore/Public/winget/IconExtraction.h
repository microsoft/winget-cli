// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <vector>
#include <winget/Manifest.h>

namespace AppInstaller::Repository
{
    struct ExtractedIconInfo
    {
        std::vector<BYTE> IconContent;
        std::vector<BYTE> IconSha256;
        AppInstaller::Manifest::IconFileTypeEnum IconFileType = AppInstaller::Manifest::IconFileTypeEnum::Unknown;
        AppInstaller::Manifest::IconThemeEnum IconTheme = AppInstaller::Manifest::IconThemeEnum::Unknown;
        AppInstaller::Manifest::IconResolutionEnum IconResolution = AppInstaller::Manifest::IconResolutionEnum::Unknown;
    };

    // The usage of iconIndex is same as usage in ExtractIconEx at
    // https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-extracticonexw
    // The icon index usage is basically:
    //     0, 1, 2        means zero-based index of the first icon to extract.
    //     -1, -2, -3     means extract icon whose resource name is 1, 2, 3 etc
    std::vector<BYTE> ExtractIconFromBinaryFile(const std::filesystem::path binaryPath, int iconIndex = 0);

    // Extracts the app icon given the app's product code.
    // This method uses similar logic for retrieving icon as app list in settings page.
    // This method returns empty if only default icons would be picked.
    // This method returns contents of .ico icons.
    std::vector<ExtractedIconInfo> ExtractIconFromArpEntry(const std::string& productCode, AppInstaller::Manifest::ScopeEnum scope = AppInstaller::Manifest::ScopeEnum::Unknown);
}
