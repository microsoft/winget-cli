// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/SQLiteIndex.h"
#include <winget/Manifest.h>

namespace AppInstaller::Repository
{
    // Validate the manifest arp version range against index. Any validation failures will be thrown as ManifestException for better message back to caller.
    void ValidateManifestArpVersion(const Microsoft::SQLiteIndex* index, const Manifest::Manifest& manifest);
}
