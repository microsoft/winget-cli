// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MatchResult.h"
#include "MatchResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::CatalogPackage MatchResult::CatalogPackage()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageMatchFilter MatchResult::MatchCriteria()
    {
        throw hresult_not_implemented();
    }
}
