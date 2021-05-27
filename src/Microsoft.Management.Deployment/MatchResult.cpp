// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <mutex>
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRepositorySearch.h>
#include "MatchResult.h"
#include "MatchResult.g.cpp"
#include "CatalogPackage.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void MatchResult::Initialize(Microsoft::Management::Deployment::CatalogPackage package, Microsoft::Management::Deployment::PackageMatchFilter matchCriteria)
    {
        m_catalogPackage = package;
        m_matchCriteria = matchCriteria;
    }
    winrt::Microsoft::Management::Deployment::CatalogPackage MatchResult::CatalogPackage()
    {
        return m_catalogPackage;
    }
    winrt::Microsoft::Management::Deployment::PackageMatchFilter MatchResult::MatchCriteria()
    {
        return m_matchCriteria;
    }
}
