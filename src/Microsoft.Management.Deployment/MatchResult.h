// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "MatchResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct MatchResult : MatchResultT<MatchResult>
    {
        MatchResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(Microsoft::Management::Deployment::CatalogPackage package, Microsoft::Management::Deployment::PackageMatchFilter matchCriteria);
#endif

        winrt::Microsoft::Management::Deployment::CatalogPackage CatalogPackage();
        winrt::Microsoft::Management::Deployment::PackageMatchFilter MatchCriteria();
        
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Microsoft::Management::Deployment::CatalogPackage m_catalogPackage{ nullptr };
        Microsoft::Management::Deployment::PackageMatchFilter m_matchCriteria{ nullptr };
#endif
    };
}
