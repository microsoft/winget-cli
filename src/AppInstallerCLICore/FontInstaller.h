// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <winget/Fonts.h>

namespace AppInstaller::CLI::Font
{
    struct FontFile
    {
        FontFile(std::filesystem::path filePath, DWRITE_FONT_FILE_TYPE fileType)
            : FilePath(std::move(filePath)), FileType(fileType) {}

        std::filesystem::path FilePath;
        DWRITE_FONT_FILE_TYPE FileType;
    };

    struct FontInstaller
    {
        FontInstaller(Manifest::ScopeEnum scope);

        std::filesystem::path FontFileLocation;

        void Install(const std::vector<FontFile>& fontFiles);

        void Uninstall(const std::wstring& familyName);

    private:
        Manifest::ScopeEnum m_scope;
        std::filesystem::path m_installLocation;
        Registry::Key m_key;
    };
}
