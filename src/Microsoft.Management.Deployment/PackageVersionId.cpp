#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "PackageVersionId.h"
#include "PackageVersionId.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    PackageVersionId::PackageVersionId(::AppInstaller::Repository::PackageVersionKey packageVersionKey)
    {
        m_packageVersionKey = packageVersionKey;
    }
    hstring PackageVersionId::AppCatalogId()
    {
        return winrt::to_hstring(m_packageVersionKey.SourceId);
    }
    hstring PackageVersionId::Version()
    {
        return winrt::to_hstring(m_packageVersionKey.Version);
    }
    hstring PackageVersionId::Channel()
    {
        return winrt::to_hstring(m_packageVersionKey.Channel);
    }
}
