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
        size_t GetNextTestPackageId()
        {
            static std::atomic_size_t packageId(0);
            return ++packageId;
        }
    }

    TestPackageVersion::TestPackageVersion(const Manifest& manifest, MetadataMap installationMetadata, std::weak_ptr<const ISource> source) :
        VersionManifest(manifest), Metadata(std::move(installationMetadata)), Source(source) {}

    TestPackageVersion::TestPackageVersion(const Manifest& manifest, std::weak_ptr<const ISource> source, bool hideSystemReferenceStrings) :
        VersionManifest(manifest), Source(source), HideSystemReferenceStrings(hideSystemReferenceStrings) {}

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
            if (!HideSystemReferenceStrings)
            {
                for (const auto& installer : VersionManifest.Installers)
                {
                    AddIfHasValueAndNotPresent(installer.PackageFamilyName, result, true);
                }
            }
            break;
        case PackageVersionMultiProperty::ProductCode:
            if (!HideSystemReferenceStrings)
            {
                for (const auto& installer : VersionManifest.Installers)
                {
                    bool shouldFoldCaseForNonPortable = installer.EffectiveInstallerType() != AppInstaller::Manifest::InstallerTypeEnum::Portable;
                    AddIfHasValueAndNotPresent(installer.ProductCode, result, shouldFoldCaseForNonPortable);
                }
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

    TestPackage::TestPackage(const std::vector<Manifest>& available, std::weak_ptr<const ISource> source, bool hideSystemReferenceStringsOnVersion) :
        Source(source)
    {
        DefaultIsSameIdentity = GetNextTestPackageId();
        for (const auto& manifest : available)
        {
            Versions.emplace_back(TestPackageVersion::Make(manifest, source, hideSystemReferenceStringsOnVersion));
        }
    }

    TestPackage::TestPackage(const Manifest& installed, MetadataMap installationMetadata, std::weak_ptr<const ISource> source) :
        Source(source)
    {
        DefaultIsSameIdentity = GetNextTestPackageId();
        Versions.emplace_back(TestPackageVersion::Make(installed, std::move(installationMetadata), source));
    }

    TestPackage::LocIndString TestPackage::GetProperty(PackageProperty property) const
    {
        std::shared_ptr<IPackageVersion> truth;

        if (!Versions.empty())
        {
            truth = Versions[0];
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

    std::vector<PackageVersionKey> TestPackage::GetVersionKeys() const
    {
        std::vector<PackageVersionKey> result;
        for (const auto& version : Versions)
        {
            result.emplace_back(PackageVersionKey(version->GetSource().GetIdentifier(), version->GetProperty(PackageVersionProperty::Version).get(), version->GetProperty(PackageVersionProperty::Channel).get()));
        }
        return result;
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetLatestVersion() const
    {
        if (Versions.empty())
        {
            return {};
        }

        return Versions[0];
    }

    std::shared_ptr<IPackageVersion> TestPackage::GetVersion(const PackageVersionKey& versionKey) const
    {
        for (const auto& version : Versions)
        {
            if ((versionKey.Version.empty() || versionKey.Version == version->GetProperty(PackageVersionProperty::Version).get()) && 
                (versionKey.Channel.empty() || versionKey.Channel == version->GetProperty(PackageVersionProperty::Channel).get()))
            {
                return version;
            }
        }

        return {};
    }

    Repository::Source TestPackage::GetSource() const
    {
        return std::const_pointer_cast<TestPackage::ISource>(Source.lock());
    }

    bool TestPackage::IsSame(const IPackage* other) const
    {
        if (IsSameOverride)
        {
            return IsSameOverride(this, other);
        }

        const TestPackage* otherPackage = PackageCast<const TestPackage*>(other);

        if (otherPackage && DefaultIsSameIdentity == otherPackage->DefaultIsSameIdentity)
        {
            return true;
        }

        return false;
    }

    const void* TestPackage::CastTo(IPackageType type) const
    {
        if (type == PackageType)
        {
            return this;
        }

        return nullptr;
    }

    TestCompositePackage::TestCompositePackage(const std::vector<Manifest>& available, std::weak_ptr<const ISource> source, bool hideSystemReferenceStringsOnVersion)
    {
        if (!available.empty())
        {
            Available.emplace_back(TestPackage::Make(available, source, hideSystemReferenceStringsOnVersion));
        }
    }

    TestCompositePackage::TestCompositePackage(const Manifest& installed, MetadataMap installationMetadata, const std::vector<Manifest>& available, std::weak_ptr<const ISource> source) :
        Installed(TestPackage::Make(installed, std::move(installationMetadata), source))
    {
        if (!available.empty())
        {
            Available.emplace_back(TestPackage::Make(available, source));
        }
    }

    TestCompositePackage::LocIndString TestCompositePackage::GetProperty(PackageProperty property) const
    {
        std::shared_ptr<IPackage> truth;

        if (!Available.empty())
        {
            truth = Available[0];
        }
        else
        {
            truth = Installed;
        }

        if (!truth)
        {
            THROW_HR(E_NOT_VALID_STATE);
        }

        switch (property)
        {
        case PackageProperty::Id:
            return truth->GetProperty(PackageProperty::Id);
        case PackageProperty::Name:
            return truth->GetProperty(PackageProperty::Name);
        default:
            return {};
        }
    }

    std::shared_ptr<AppInstaller::Repository::IPackage> TestCompositePackage::GetInstalled()
    {
        return Installed;
    }

    std::vector<std::shared_ptr<AppInstaller::Repository::IPackage>> TestCompositePackage::GetAvailable()
    {
        return { Available.begin(), Available.end() };
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

    bool TestSource::QueryFeatureFlag(SourceFeatureFlag flag) const
    {
        return (QueryFeatureFlagFunction ? QueryFeatureFlagFunction(flag) : false);
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
        std::shared_ptr<TestSourceReference> result;

        if (OnOpenWithCustomHeader)
        {
            result = std::make_shared<TestSourceReference>(details, OnOpenWithCustomHeader);
        }
        else
        {
            result = std::make_shared<TestSourceReference>(details, OnOpen);
        }

        result->ShouldUpdateBeforeOpenResult = ShouldUpdateBeforeOpenResult;

        return result;
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
        Repository::Source source{ details.Name, details.Arg, details.Type, Repository::SourceTrustLevel::None, false };
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
