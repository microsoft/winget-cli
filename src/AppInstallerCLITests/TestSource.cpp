// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"

using namespace AppInstaller;
using namespace AppInstaller::Repository;

namespace TestCommon
{
    TestPackageVersion::TestPackageVersion(const Manifest& manifest, MetadataMap installationMetadata) :
        VersionManifest(manifest), Metadata(std::move(installationMetadata)) {}

    TestPackageVersion::LocIndString TestPackageVersion::GetProperty(PackageVersionProperty property) const
    {
        switch (property)
        {
        case PackageVersionProperty::Id:
            return LocIndString{ VersionManifest.Id };
        case PackageVersionProperty::Name:
            return LocIndString{ VersionManifest.Name };
        case PackageVersionProperty::Version:
            return LocIndString{ VersionManifest.Version };
        case PackageVersionProperty::Channel:
            return LocIndString{ VersionManifest.Channel };
        default:
            return {};
        }
    }

    std::vector<TestPackageVersion::LocIndString> TestPackageVersion::GetMultiProperty(PackageVersionMultiProperty property) const
    {
        std::vector<LocIndString> result;

        switch (property)
        {
        case PackageVersionMultiProperty::PackageFamilyName:
            for (const auto& installer : VersionManifest.Installers)
            {
                AddFoldedIfHasValueAndNotPresent(installer.PackageFamilyName, result);
            }
            break;
        case PackageVersionMultiProperty::ProductCode:
            for (const auto& installer : VersionManifest.Installers)
            {
                AddFoldedIfHasValueAndNotPresent(installer.ProductCode, result);
            }
            break;
        }

        return result;
    }

    TestPackageVersion::Manifest TestPackageVersion::GetManifest() const
    {
        return VersionManifest;
    }

    std::shared_ptr<const ISource> TestPackageVersion::GetSource() const
    {
        return Source;
    }

    TestPackageVersion::MetadataMap TestPackageVersion::GetMetadata() const
    {
        return Metadata;
    }

    void TestPackageVersion::AddFoldedIfHasValueAndNotPresent(const Utility::NormalizedString& value, std::vector<LocIndString>& target)
    {
        if (!value.empty())
        {
            std::string folded = FoldCase(value);
            auto itr = std::find(target.begin(), target.end(), folded);
            if (itr == target.end())
            {
                target.emplace_back(std::move(folded));
            }
        }
    }

    TestPackage::TestPackage(const std::vector<Manifest>& available)
    {
        for (const auto& manifest : available)
        {
            AvailableVersions.emplace_back(TestPackageVersion::Make(manifest));
        }
    }

    TestPackage::TestPackage(const Manifest& installed, MetadataMap installationMetadata, const std::vector<Manifest>& available) :
        InstalledVersion(TestPackageVersion::Make(installed, std::move(installationMetadata)))
    {
        for (const auto& manifest : available)
        {
            AvailableVersions.emplace_back(TestPackageVersion::Make(manifest));
        }
    }

    TestPackage::LocIndString TestPackage::GetProperty(PackageProperty property) const
    {
        std::shared_ptr<IPackageVersion> truth;

        if (!AvailableVersions.empty())
        {
            truth = AvailableVersions[0];
        }
        else
        {
            truth = InstalledVersion;
        }

        if (!truth)
        {
            THROW_HR(E_NOT_VALID_STATE);
        }

        switch (property)
        {
        case PackageProperty::Id:
            return truth->GetProperty(PackageVersionProperty::Id);
        case PackageProperty::Name:
            return truth->GetProperty(PackageVersionProperty::Name);
        default:
            return {};
        }
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetInstalledVersion() const
    {
        return InstalledVersion;
    }

    std::vector<PackageVersionKey> TestPackage::GetAvailableVersionKeys() const
    {
        std::vector<PackageVersionKey> result;
        for (const auto& version : AvailableVersions)
        {
            result.emplace_back(PackageVersionKey("", version->GetProperty(PackageVersionProperty::Version).get(), version->GetProperty(PackageVersionProperty::Channel).get()));
        }
        return result;
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetLatestAvailableVersion() const
    {
        if (AvailableVersions.empty())
        {
            return {};
        }

        return AvailableVersions[0];
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetAvailableVersion(const PackageVersionKey& versionKey) const
    {
        for (const auto& version : AvailableVersions)
        {
            if ((versionKey.Version.empty() || versionKey.Version == version->GetProperty(PackageVersionProperty::Version).get()) && 
                (versionKey.Channel.empty() || versionKey.Channel == version->GetProperty(PackageVersionProperty::Channel).get()))
            {
                return version;
            }
        }

        return {};
    }

    bool TestPackage::IsUpdateAvailable() const
    {
        if (InstalledVersion && !AvailableVersions.empty())
        {
            Utility::Version installed{ InstalledVersion->GetProperty(PackageVersionProperty::Version) };
            Utility::Version available{ AvailableVersions[0]->GetProperty(PackageVersionProperty::Version) };

            return available > installed;
        }

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

    SearchResult TestSource::Search(const SearchRequest& request) const
    {
        if (SearchFunction)
        {
            return SearchFunction(request);
        }
        else
        {
            return {};
        }
    }

    bool TestSource::IsComposite() const
    {
        return Composite;
    }
}
