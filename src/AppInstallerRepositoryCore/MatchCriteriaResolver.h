// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/winget/RepositorySearch.h"

namespace AppInstaller::Repository
{
    // Finds the highest rated match criteria for the package based on the search request,
    PackageMatchFilter FindBestMatchCriteria(const SearchRequest& request, const IPackageVersion* packageVersion);
}
