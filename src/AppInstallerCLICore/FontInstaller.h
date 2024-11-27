// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>

namespace AppInstaller::CLI::Font
{
    // Object representation of the metadata and functionality required for installing a font package. 
    struct FontInstaller
    {
        std::filesystem::path FontFileLocation;

        FontInstaller(const std::string& familyName, Manifest::ScopeEnum scope);

        void Install();

    private:
        Manifest::ScopeEnum m_scope;
        std::string m_familyName;
    };
}
