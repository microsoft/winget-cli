// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "PackageCatalogInfo.h"
#include "PackageCatalogInfo.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageCatalogInfo::Initialize(const ::AppInstaller::Repository::SourceDetails& sourceDetails)
    {
        m_sourceDetails = sourceDetails;
    }
    ::AppInstaller::Repository::SourceDetails PackageCatalogInfo::GetSourceDetails()
    {
        return m_sourceDetails;
    }
    hstring PackageCatalogInfo::Id()
    {
        return winrt::to_hstring(m_sourceDetails.Identifier);
    }
    hstring PackageCatalogInfo::Name()
    {
        return winrt::to_hstring(m_sourceDetails.Name);
    }
    hstring PackageCatalogInfo::Type()
    {
        return winrt::to_hstring(m_sourceDetails.Type);
    }
    hstring PackageCatalogInfo::Argument()
    {
        return winrt::to_hstring(m_sourceDetails.Arg);
    }
    winrt::Windows::Foundation::DateTime PackageCatalogInfo::LastUpdateTime()
    {
        return winrt::clock::from_time_t(std::chrono::system_clock::to_time_t(m_sourceDetails.LastUpdateTime));
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogOrigin PackageCatalogInfo::Origin()
    {
        switch (m_sourceDetails.Origin)
        {
        case ::AppInstaller::Repository::SourceOrigin::Default :
        case ::AppInstaller::Repository::SourceOrigin::Predefined:
            return PackageCatalogOrigin::Predefined;
        case ::AppInstaller::Repository::SourceOrigin::User:
        case ::AppInstaller::Repository::SourceOrigin::GroupPolicy:
        default:
            return PackageCatalogOrigin::User;
        }
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel PackageCatalogInfo::TrustLevel()
    {
        switch (m_sourceDetails.TrustLevel)
        {
        case ::AppInstaller::Repository::SourceTrustLevel::Trusted:
            return PackageCatalogTrustLevel::Trusted;
        case ::AppInstaller::Repository::SourceTrustLevel::None:
        default:
            return PackageCatalogTrustLevel::None;
        }
    }
}
