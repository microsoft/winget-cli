// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>

namespace AppInstaller::CLI::Font
{
    struct FontInstaller
    {
        FontInstaller(Manifest::ScopeEnum scope);

        std::filesystem::path FontFileLocation;

        void Install(const std::map<std::wstring, std::filesystem::path> fontFiles);

    private:
        Manifest::ScopeEnum m_scope;
        std::filesystem::path m_installLocation;
        Registry::Key m_key;
    };
}
