// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"

using namespace AppInstaller;
using namespace AppInstaller::Repository;

namespace TestCommon
{
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
        case PackageVersionProperty::Publisher:
            return LocIndString{ VersionManifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::Publisher>() };
        case PackageVersionProperty::ArpMinVersion:
            return LocIndString{ VersionManifest.GetArpVersionRange().IsEmpty() ? "" : VersionManifest.GetArpVersionRange().GetMinVersion().ToString() };
        case PackageVersionProperty::ArpMaxVersion:
            return LocIndString{ VersionManifest.GetArpVersionRange().IsEmpty() ? "" : VersionManifest.GetArpVersionRange().GetMaxVersion().ToString() };
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
                AddIfHasValueAndNotPresent(installer.PackageFamilyName, result, true);
            }
            break;
        case PackageVersionMultiProperty::ProductCode:
            for (const auto& installer : VersionManifest.Installers)
            {
                bool shouldFoldCaseForNonPortable = installer.EffectiveInstallerType() != AppInstaller::Manifest::InstallerTypeEnum::Portable;
                AddIfHasValueAndNotPresent(installer.ProductCode, result, shouldFoldCaseForNonPortable);
            }
            break;
        case PackageVersionMultiProperty::Name:
            for (auto name : VersionManifest.GetPackageNames())
            {
                result.emplace_back(std::move(name));
            }
            break;
        case PackageVersionMultiProperty::Publisher:
            for (auto publisher : VersionManifest.GetPublishers())
            {
                result.emplace_back(std::move(publisher));
            }
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

    Repository::Source TestPackageVersion::GetSource() const
    {
        return std::const_pointer_cast<TestPackageVersion::ISource>(Source.lock());
    }

    TestPackageVersion::MetadataMap TestPackageVersion::GetMetadata() const
    {
        return Metadata;
    }

    void TestPackageVersion::AddIfHasValueAndNotPresent(const Utility::NormalizedString& value, std::vector<LocIndString>& target, bool folded)
    {
        if (!value.empty())
        {
            std::string valueString = folded ? FoldCase(value) : value;
            auto itr = std::find(target.begin(), target.end(), valueString);
            if (itr == target.end())
            {
                target.emplace_back(std::move(valueString));
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

    std::vector<PackageVersionKey> TestPackage::GetAvailableVersionKeys(PinBehavior) const
    {
        std::vector<PackageVersionKey> result;
        for (const auto& version : AvailableVersions)
        {
            result.emplace_back(PackageVersionKey(version->GetSource().GetIdentifier(), version->GetProperty(PackageVersionProperty::Version).get(), version->GetProperty(PackageVersionProperty::Channel).get()));
        }
        return result;
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetLatestAvailableVersion(PinBehavior) const
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

    bool TestPackage::IsUpdateAvailable(PinBehavior) const
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
        if (IsSameOverride)
        {
            return IsSameOverride(this, other);
        }

        const TestPackage* otherAvailable = PackageCast<const TestPackage*>(other);

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

    const void* TestPackage::CastTo(IPackageType type) const
    {
        if (type == PackageType)
        {
            return this;
        }

        return nullptr;
    }

    const SourceDetails& TestSource::GetDetails() const
    {
        return Details;
    }

    const std::string& TestSource::GetIdentifier() const
    {
        return Details.Identifier;
    }

    SourceInformation TestSource::GetInformation() const
    {
        return Information;
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

    void* TestSource::CastTo(AppInstaller::Repository::ISourceType type)
    {
        if (type == SourceType)
        {
            return this;
        }

        return nullptr;
    }

    std::string_view TestSourceFactory::TypeName() const
    {
        return "*TestSource"sv;
    }

    std::shared_ptr<ISourceReference> TestSourceFactory::Create(const SourceDetails& details)
    {
        if (OnOpenWithCustomHeader)
        {
            return std::make_shared<TestSourceReference>(details, OnOpenWithCustomHeader);
        }
        else
        {
            return std::make_shared<TestSourceReference>(details, OnOpen);
        }
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

    bool AddSource(const AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback& progress)
    {
        Repository::Source source{ details.Name, details.Arg, details.Type };
        return source.Add(progress);
    }

    bool UpdateSource(std::string_view name, AppInstaller::IProgressCallback& progress)
    {
        Repository::Source source{ name };
        return source.Update(progress).empty();
    }

    bool RemoveSource(std::string_view name, AppInstaller::IProgressCallback& progress)
    {
        Repository::Source source{ name };
        return source.Remove(progress);
    }

    std::vector<AppInstaller::Repository::SourceDetails> GetSources()
    {
        return Repository::Source::GetCurrentSources();
    }

    AppInstaller::Repository::Source OpenSource(std::string_view name, AppInstaller::IProgressCallback& progress)
    {
        Repository::Source source{ name };
        source.Open(progress);
        return source;
    }

    void DropSource(std::string_view name)
    {
        Source::DropSource(name);
    }
}
