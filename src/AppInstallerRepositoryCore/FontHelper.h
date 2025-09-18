// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/SQLiteIndex.h"
#include <winget/Registry.h>
#include <winget/ManifestInstaller.h>
#include <winget/Fonts.h>
#include <wil/registry.h>
#include <wil/resource.h>

#include <string>
#include <vector>

namespace AppInstaller::Repository::Microsoft
{
    // A helper to find the various locations that find installed font information.
    struct FontHelper
    {
        void PopulateIndex(SQLiteIndex& index, Manifest::ScopeEnum scope) const;
    };
}
