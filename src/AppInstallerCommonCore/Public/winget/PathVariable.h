// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winget/Registry.h"
#include "winget/Manifest.h"

namespace AppInstaller::Registry::Environment
{
    struct PathVariable
    {
        PathVariable(Manifest::ScopeEnum scope);

        // Returns the PATH variable as a string.
        std::string GetPathValue();

        // Checks if the PATH variable contains the target path.
        bool Contains(const std::filesystem::path& target);

        // Returns a value indicating whether the target path was removed from the PATH variable.
        bool Remove(const std::filesystem::path& target);

        // Returns a value indicating whether the target path was appended to the PATH variable.
        bool Append(const std::filesystem::path& target);

    private:
        void SetPathValue(const std::string& value);
        Registry::Key m_key;
        HKEY m_root;
        Manifest::ScopeEnum m_scope;
    };
}