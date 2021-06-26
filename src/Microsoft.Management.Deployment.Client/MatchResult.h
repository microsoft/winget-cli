// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "MatchResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct MatchResult : MatchResultT<MatchResult>
    {
        MatchResult() = default;

        winrt::Microsoft::Management::Deployment::CatalogPackage CatalogPackage();
        winrt::Microsoft::Management::Deployment::PackageMatchFilter MatchCriteria();
    };
}
