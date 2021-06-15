// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"

using namespace AppInstaller;
using namespace AppInstaller::Repository;

namespace TestCommon
{
    namespace
    {
        template<AppInstaller::Manifest::Localization Field>
        void BuildPackageVersionMultiPropertyWithFallback(std::vector<Utility::LocIndString>& result, const Manifest::Manifest& VersionManifest)
        {
            result.emplace_back(VersionManifest.DefaultLocalization.Get<Field>());
            for (const auto& loc : VersionManifest.Localizations)
            {
                auto f = loc.Get<Field>();
                if (f.empty())
                {
                    result.emplace_back(loc.Get<Field>());
                }
                else
                {
                    result.emplace_back(std::move(f));
                }
            }
        }
    }

    TestPackageVersion::TestPackageVersion(const Manifest& manifest, MetadataMap installationMetadata, std::weak_ptr<const ISource> source) :
        VersionManifest(manifest), Metadata(std::move(installationMetadata)), Source(source) {}

    TestPackageVersion::TestPackageVersion(const Manifest& manifest, std::weak_ptr<const ISource> source) :
        VersionManifest(manifest), Source(source) {}

    TestPackageVersion::LocIndString TestPackageVersion::GetProperty(PackageVersionProperty property) const
    {
        switch (property)
        {
        case PackageVersionProperty::Id:
            return LocIndString{ VersionManifest.Id };
        case PackageVersionProperty::Name:
            return LocIndString{ VersionManifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::PackageName>() };
        case PackageVersionProperty::Version:
            return LocIndString{ VersionManifest.Version };
        case PackageVersionProperty::Channel:
            return LocIndString{ VersionManifest.Channel };
        case PackageVersionProperty::SourceIdentifier:
            return LocIndString{ Source.lock()->GetIdentifier() };
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
        case PackageVersionMultiProperty::Name:
            BuildPackageVersionMultiPropertyWithFallback<AppInstaller::Manifest::Localization::PackageName>(result, VersionManifest);
            break;
        case PackageVersionMultiProperty::Publisher:
            BuildPackageVersionMultiPropertyWithFallback<AppInstaller::Manifest::Localization::Publisher>(result, VersionManifest);
            break;
        case PackageVersionMultiProperty::Locale:
            result.emplace_back(VersionManifest.DefaultLocalization.Locale);
            for (const auto& loc : VersionManifest.Localizations)
            {
                result.emplace_back(loc.Locale);
            }
            break;
        }

        return result;
    }

    TestPackageVersion::Manifest TestPackageVersion::GetManifest()
    {
        return VersionManifest;
    }

    std::shared_ptr<const ISource> TestPackageVersion::GetSource() const
    {
        return Source.lock();
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

    TestPackage::TestPackage(const std::vector<Manifest>& available, std::weak_ptr<const ISource> source)
    {
        for (const auto& manifest : available)
        {
            AvailableVersions.emplace_back(TestPackageVersion::Make(manifest, source));
        }
    }

    TestPackage::TestPackage(const Manifest& installed, MetadataMap installationMetadata, const std::vector<Manifest>& available, std::weak_ptr<const ISource> source) :
        InstalledVersion(TestPackageVersion::Make(installed, std::move(installationMetadata), source))
    {
        for (const auto& manifest : available)
        {
            AvailableVersions.emplace_back(TestPackageVersion::Make(manifest, source));
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

    bool TestPackage::IsSame(const IPackage* other) const
    {
        const TestPackage* otherAvailable = dynamic_cast<const TestPackage*>(other);

        if (!otherAvailable ||
            InstalledVersion.get() != otherAvailable->InstalledVersion.get() ||
            AvailableVersions.size() != otherAvailable->AvailableVersions.size())
        {
            return false;
        }

        for (size_t i = 0; i < AvailableVersions.size(); ++i)
        {
            if (AvailableVersions[i].get() != otherAvailable->AvailableVersions[i].get())
            {
                return false;
            }
        }

        return true;
    }

    const SourceDetails& TestSource::GetDetails() const
    {
        return Details;
    }

    const std::string& TestSource::GetIdentifier() const
    {
        return Details.Identifier;
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

    std::shared_ptr<ISource> TestSourceFactory::Create(const SourceDetails& details, IProgressCallback&)
    {
        return OnCreate(details);
    }

    bool TestSourceFactory::Add(SourceDetails& details, IProgressCallback&)
    {
        if (OnAdd)
        {
            OnAdd(details);
        }
        return true;
    }

    bool TestSourceFactory::Update(const SourceDetails& details, IProgressCallback&)
    {
        if (OnUpdate)
        {
            OnUpdate(details);
        }
        return true;
    }

    bool TestSourceFactory::Remove(const SourceDetails& details, IProgressCallback&)
    {
        if (OnRemove)
        {
            OnRemove(details);
        }
        return true;
    }

    // Make copies of self when requested.
    TestSourceFactory::operator std::function<std::unique_ptr<ISourceFactory>()>()
    {
        return [this]() { return std::make_unique<TestSourceFactory>(*this); };
    }
}
