// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <winget/Fonts.h>

namespace AppInstaller::CLI::Font
{
    struct FontFile
    {
        FontFile(std::filesystem::path filePath, DWRITE_FONT_FILE_TYPE fileType);

        std::filesystem::path FilePath;
        DWRITE_FONT_FILE_TYPE FileType;
        std::wstring Title;
        std::filesystem::path DestinationPath;
    };

    struct FontInstaller
    {
        FontInstaller(Manifest::ScopeEnum scope);

        std::filesystem::path FontFileLocation;

        void SetFontFiles(const std::vector<FontFile>& fontFiles)
        {
            m_fontFiles = fontFiles;
        }

        // Checks if all expected registry values and font files can be installed prior to installation.
        bool EnsureInstall();

        void Install();

        void Uninstall();

    private:
        Manifest::ScopeEnum m_scope;
        std::filesystem::path m_installLocation;
        Registry::Key m_key;

        std::vector<FontFile> m_fontFiles;
    };
}
