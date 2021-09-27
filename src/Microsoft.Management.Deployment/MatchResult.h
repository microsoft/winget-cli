// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "MatchResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct MatchResult : MatchResultT<MatchResult>
    {
        MatchResult() = default;
        void Initialize(Microsoft::Management::Deployment::CatalogPackage package, Microsoft::Management::Deployment::PackageMatchFilter matchCriteria);

        winrt::Microsoft::Management::Deployment::CatalogPackage CatalogPackage();
        winrt::Microsoft::Management::Deployment::PackageMatchFilter MatchCriteria();
    private:
        Microsoft::Management::Deployment::CatalogPackage m_catalogPackage{ nullptr };
        Microsoft::Management::Deployment::PackageMatchFilter m_matchCriteria{ nullptr };
    };
}
