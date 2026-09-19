// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/winget/RepositorySearch.h"
#include <functional>

namespace AppInstaller::Repository
{
    // Returns whether the value matches, or nullopt if the match type cannot be evaluated locally.
    std::optional<bool> MatchesRequest(const RequestMatch& request, const Utility::NormalizedString& value);

    // Applies field-specific casing rules when comparing a value.
    std::optional<bool> MatchesRequest(const PackageMatchFilter& request, const Utility::NormalizedString& value);

    // Evaluates a field using complete manifest data; unsupported fields and match types remain unknown.
    std::optional<bool> MatchesRequest(const PackageMatchFilter& request, const Manifest::Manifest& manifest);

    // Combines field matches using request semantics; source-defined queries remain unknown.
    std::optional<bool> MatchesRequest(const SearchRequest& request,
        const std::function<std::optional<bool>(const PackageMatchFilter&)>& matchesField);

    // Finds the highest rated match criteria for the package based on the search request,
    PackageMatchFilter FindBestMatchCriteria(const SearchRequest& request, const IPackageVersion* packageVersion);
}
