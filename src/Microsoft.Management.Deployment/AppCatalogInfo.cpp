#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "AppCatalogInfo.h"
#include "AppCatalogInfo.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    AppCatalogInfo::AppCatalogInfo(::AppInstaller::Repository::SourceDetails sourceDetails)
    {
        m_sourceDetails = sourceDetails;
    }
    hstring AppCatalogInfo::Id()
    {
        return winrt::to_hstring(m_sourceDetails.Identifier);
    }
    hstring AppCatalogInfo::Name()
    {
        return winrt::to_hstring(m_sourceDetails.Name);
    }
    hstring AppCatalogInfo::Type()
    {
        return winrt::to_hstring(m_sourceDetails.Type);
    }
    hstring AppCatalogInfo::Arg()
    {
        return winrt::to_hstring(m_sourceDetails.Arg);
    }
    hstring AppCatalogInfo::ExtraData()
    {
        return winrt::to_hstring(m_sourceDetails.Data);
    }
    Windows::Foundation::DateTime AppCatalogInfo::LastUpdateTime()
    {
        return winrt::clock::from_time_t(std::chrono::system_clock::to_time_t(m_sourceDetails.LastUpdateTime));
    }
    Microsoft::Management::Deployment::AppCatalogOrigin AppCatalogInfo::Origin()
    {
        switch (m_sourceDetails.Origin)
        {
        case ::AppInstaller::Repository::SourceOrigin::Default :
        case ::AppInstaller::Repository::SourceOrigin::Predefined:
            return AppCatalogOrigin::Predefined;
        case ::AppInstaller::Repository::SourceOrigin::User:
        case ::AppInstaller::Repository::SourceOrigin::GroupPolicy:
        default:
            return AppCatalogOrigin::User;
        }
    }
    Microsoft::Management::Deployment::AppCatalogTrustLevel AppCatalogInfo::TrustLevel()
    {
        switch (m_sourceDetails.TrustLevel)
        {
        case ::AppInstaller::Repository::SourceTrustLevel::Trusted:
            return AppCatalogTrustLevel::Trusted;
        case ::AppInstaller::Repository::SourceTrustLevel::None:
        default:
            return AppCatalogTrustLevel::None;
        }
    }
}
