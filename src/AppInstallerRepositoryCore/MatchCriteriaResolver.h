// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/winget/RepositorySearch.h"

namespace AppInstaller::Repository
{
    // Returns whether the value matches, or nullopt if the match type cannot be evaluated locally.
    std::optional<bool> MatchesRequest(const RequestMatch& request, const Utility::NormalizedString& value);

    // Finds the highest rated match criteria for the package based on the search request,
    PackageMatchFilter FindBestMatchCriteria(const SearchRequest& request, const IPackageVersion* packageVersion);
}
