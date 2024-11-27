// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "FontInstaller.h"
#include <winget/Manifest.h>
#include <winget/ManifestCommon.h>
#include <winget/Filesystem.h>
#include <AppInstallerErrors.h>
#include <AppInstallerRuntime.h>

namespace AppInstaller::CLI::Font
{
    FontInstaller::FontInstaller(const std::string& familyName, Manifest::ScopeEnum scope)
    {
        m_scope = scope;
        m_familyName = familyName;

        // Get the expected state from the family name and scope. 
    }

    void FontInstaller::Install()
    {
    }
}
