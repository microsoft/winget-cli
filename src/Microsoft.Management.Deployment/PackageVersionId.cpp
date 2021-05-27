// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "PackageVersionId.h"
#include "PackageVersionId.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageVersionId::Initialize(::AppInstaller::Repository::PackageVersionKey packageVersionKey)
    {
        m_packageVersionKey = packageVersionKey;
    }
    hstring PackageVersionId::PackageCatalogId()
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
