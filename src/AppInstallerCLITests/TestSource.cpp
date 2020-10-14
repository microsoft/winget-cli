// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

namespace TestCommon
{
    TestPackageVersion::TestPackageVersion(const Manifest& manifest) : m_manifest(manifest) {}

    LocIndString TestPackageVersion::GetProperty(PackageVersionProperty property) const
    {
        switch (property)
        {
        case PackageVersionProperty::Id:
            return LocIndString{ m_manifest.Id };
        case PackageVersionProperty::Name:
            return LocIndString{ m_manifest.Name };
        case PackageVersionProperty::Version:
            return LocIndString{ m_manifest.Version };
        case PackageVersionProperty::Channel:
            return LocIndString{ m_manifest.Channel };
        default:
            return {};
        }
    }

    std::vector<LocIndString> TestPackageVersion::GetMultiProperty(PackageVersionMultiProperty property) const
    {
        switch (property)
        {
        case PackageVersionMultiProperty::PackageFamilyName:
        case PackageVersionMultiProperty::ProductCode:
        default:
            return {};
        }
    }

    Manifest TestPackageVersion::GetManifest() const
    {
        return m_manifest;
    }

    std::map<std::string, std::string> TestPackageVersion::GetInstallationMetadata() const
    {
        return {};
    }

    TestPackage::TestPackage(const Manifest& manifest) : m_manifest(manifest) {}

    LocIndString TestPackage::GetProperty(PackageProperty property) const
    {
        switch (property)
        {
        case PackageProperty::Id:
            return LocIndString{ m_manifest.Id };
        case PackageProperty::Name:
            return LocIndString{ m_manifest.Name };
        default:
            return {};
        }
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetInstalledVersion() const
    {
        return {};
    }

    std::vector<PackageVersionKey> TestPackage::GetAvailableVersionKeys() const
    {
        return { { "", m_manifest.Version, m_manifest.Channel } };
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetLatestAvailableVersion() const
    {
        return std::make_shared<TestPackageVersion>(m_manifest);
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetAvailableVersion(const PackageVersionKey& versionKey) const
    {
        if ((versionKey.Version.empty() || versionKey.Version == m_manifest.Version) &&
            (versionKey.Channel.empty() || versionKey.Channel == m_manifest.Channel))
        {
            return std::make_shared<TestPackageVersion>(m_manifest);
        }
        else
        {
            return {};
        }
    }

    bool TestPackage::IsUpdateAvailable() const
    {
        return false;
    }

    const SourceDetails& TestSource::GetDetails() const
    {
        return Details;
    }

    const std::string& TestSource::GetIdentifier() const
    {
        return Identifier;
    }

    SearchResult TestSource::Search(const SearchRequest&) const
    {
        SearchResult result;
        return result;
    }

    bool TestSource::IsComposite() const
    {
        return Composite;
    }
}
