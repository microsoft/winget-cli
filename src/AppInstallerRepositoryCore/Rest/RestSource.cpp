// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestSource.h"

using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Rest
{
    namespace
    {
        using namespace AppInstaller::Repository::Rest::Schema;

        // The source reference used by package objects.
        struct SourceReference
        {
            SourceReference(const std::shared_ptr<RestSource>& source) :
                m_source(source) {}

        protected:
            std::shared_ptr<RestSource> GetReferenceSource() const
            {
                std::shared_ptr<RestSource> source = m_source.lock();
                THROW_HR_IF(E_NOT_VALID_STATE, !source);
                return source;
            }

        private:
            std::weak_ptr<RestSource> m_source;
        };

        // The IPackage implementation for Available packages from RestSource.
        struct RestPackage : public std::enable_shared_from_this<RestPackage>, public SourceReference, public IPackage, public ICompositePackage
        {
            static constexpr IPackageType PackageType = IPackageType::RestPackage;

            RestPackage(const std::shared_ptr<RestSource>& source, IRestClient::Package&& package) :
                SourceReference(source), m_package(std::move(package))
            {
                SortVersionsInternal();
            }

            // Inherited via IPackage
            Utility::LocIndString GetProperty(PackageProperty property) const override
            {
                switch (property)
                {
                case PackageProperty::Id:
                    return Utility::LocIndString{ m_package.PackageInformation.PackageIdentifier };
                case PackageProperty::Name:
                    return Utility::LocIndString{ m_package.PackageInformation.PackageName };
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }

            std::vector<PackageVersionKey> GetVersionKeys() const override
            {
                std::shared_ptr<const RestSource> source = GetReferenceSource();
                std::scoped_lock versionsLock{ m_packageVersionsLock };

                std::vector<PackageVersionKey> result;
                for (const auto& versionInfo : m_package.Versions)
                {
                    result.emplace_back(
                        source->GetIdentifier(), versionInfo.VersionAndChannel.GetVersion().ToString(), versionInfo.VersionAndChannel.GetChannel().ToString());
                }

                return result;
            }

            std::shared_ptr<IPackageVersion> GetLatestVersion() const override
            {
                std::scoped_lock versionsLock{ m_packageVersionsLock };
                return GetLatestVersionInternal();
            }

            std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey& versionKey) const override;

            Source GetSource() const override
            {
                return Source{ GetReferenceSource() };
            }

            bool IsSame(const IPackage* other) const override
            {
                const RestPackage* otherPackage = PackageCast<const RestPackage*>(other);

                if (otherPackage)
                {
                    return GetReferenceSource()->IsSame(otherPackage->GetReferenceSource().get()) &&
                        Utility::CaseInsensitiveEquals(m_package.PackageInformation.PackageIdentifier, otherPackage->m_package.PackageInformation.PackageIdentifier);
                }

                return false;
            }

            const void* CastTo(IPackageType type) const override
            {
                if (type == PackageType)
                {
                    return this;
                }

                return nullptr;
            }

            // Inherited via ICompositePackage
            std::shared_ptr<IPackage> GetInstalled() override
            {
                return {};
            }

            std::vector<std::shared_ptr<IPackage>> GetAvailable() override
            {
                return std::vector<std::shared_ptr<IPackage>>{ shared_from_this() };
            }

            // Helpers for PackageVersion interop
            const IRestClient::PackageInfo& PackageInfo() const
            {
                return m_package.PackageInformation;
            }

            // This function is designed to handle the case where the only version that is returned by the
            // initial search is Unknown. In that case, we perform a search intended to trigger the optimized
            // path and directly get all manifests.
            bool HandleSingleUnknownVersion(IRestClient::VersionInfo& versionInfo)
            {
                // If the calling version is unknown then we want to update it if we already
                // have the results in the package.
                if (versionInfo.VersionAndChannel.GetVersion().IsUnknown() && !versionInfo.Manifest)
                {
                    std::scoped_lock versionsLock{ m_packageVersionsLock };
                    if (m_package.Versions.size() == 1 && m_package.Versions[0].VersionAndChannel.GetVersion().IsUnknown() && !m_package.Versions[0].Manifest)
                    {
                        SearchRequest request;
                        request.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, m_package.PackageInformation.PackageIdentifier);

                        IRestClient::SearchResult result = GetReferenceSource()->GetRestClient().Search(request);

                        if (result.Matches.size() == 1)
                        {
                            m_package.Versions = std::move(result.Matches[0].Versions);
                            SortVersionsInternal();
                        }
                        else
                        {
                            // Unexpected, but just leave things as they are
                            AICLI_LOG(Repo, Warning, << "Found " << result.Matches.size() << " matches for optimized search of " << m_package.PackageInformation.PackageIdentifier);
                        }
                    }

                    if (!m_package.Versions.empty())
                    {
                        // The results are now sorted; either take the last one if it is unknown
                        // or the first one if it is not (aka latest).
                        if (m_package.Versions.back().VersionAndChannel.GetVersion().IsUnknown())
                        {
                            versionInfo = m_package.Versions.back();
                        }
                        else
                        {
                            versionInfo = m_package.Versions.front();
                        }
                    }

                    return true;
                }

                return false;
            }

        private:
            std::shared_ptr<RestPackage> NonConstSharedFromThis() const
            {
                return const_cast<RestPackage*>(this)->shared_from_this();
            }

            // Must hold m_packageVersionsLock while calling this
            std::shared_ptr<IPackageVersion> GetLatestVersionInternal() const;

            // Must hold m_packageVersionsLock while calling this
            void SortVersionsInternal()
            {
                std::sort(m_package.Versions.begin(), m_package.Versions.end(),
                    [](const IRestClient::VersionInfo& a, const IRestClient::VersionInfo& b)
                    {
                        return a.VersionAndChannel < b.VersionAndChannel;
                    });
            }

            IRestClient::Package m_package;
            // Protects access to m_package.Versions
            mutable std::mutex m_packageVersionsLock;
        };

        // The IPackageVersion impl for RestSource.
        struct PackageVersion : public SourceReference, public IPackageVersion
        {
            PackageVersion(
                const std::shared_ptr<RestSource>& source, std::shared_ptr<RestPackage>&& package, IRestClient::VersionInfo versionInfo)
                : SourceReference(source), m_package(std::move(package)), m_versionInfo(std::move(versionInfo)) {}

            // Inherited via IPackageVersion
            Utility::LocIndString GetProperty(PackageVersionProperty property) const override
            {
                switch (property)
                {
                case PackageVersionProperty::SourceIdentifier:
                    return Utility::LocIndString{ GetReferenceSource()->GetIdentifier() };
                case PackageVersionProperty::SourceName:
                    return Utility::LocIndString{ GetReferenceSource()->GetDetails().Name };
                case PackageVersionProperty::Id:
                    return Utility::LocIndString{ m_package->PackageInfo().PackageIdentifier };
                case PackageVersionProperty::Name:
                    return Utility::LocIndString{ m_package->PackageInfo().PackageName };
                case PackageVersionProperty::Version:
                    return Utility::LocIndString{ m_versionInfo.VersionAndChannel.GetVersion().ToString() };
                case PackageVersionProperty::Channel:
                    return Utility::LocIndString{ m_versionInfo.VersionAndChannel.GetChannel().ToString() };
                case PackageVersionProperty::Publisher:
                    return Utility::LocIndString{ m_package->PackageInfo().Publisher };
                case PackageVersionProperty::ArpMinVersion:
                    if (!m_versionInfo.ArpVersions.empty())
                    {
                        return Utility::LocIndString{ m_versionInfo.ArpVersions.front().ToString() };
                    }
                    else if (m_versionInfo.Manifest)
                    {
                        auto arpVersionRange = m_versionInfo.Manifest->GetArpVersionRange();
                        return arpVersionRange.IsEmpty() ? Utility::LocIndString{} : Utility::LocIndString{ arpVersionRange.GetMinVersion().ToString() };
                    }
                    else
                    {
                        return {};
                    }
                case PackageVersionProperty::ArpMaxVersion:
                    if (!m_versionInfo.ArpVersions.empty())
                    {
                        return Utility::LocIndString{ m_versionInfo.ArpVersions.back().ToString() };
                    }
                    else if (m_versionInfo.Manifest)
                    {
                        auto arpVersionRange = m_versionInfo.Manifest->GetArpVersionRange();
                        return arpVersionRange.IsEmpty() ? Utility::LocIndString{} : Utility::LocIndString{ arpVersionRange.GetMaxVersion().ToString() };
                    }
                    else
                    {
                        return {};
                    }
                default:
                    return {};
                }
            }

            std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
            {
                std::vector<Utility::LocIndString> result;
                switch (property)
                {
                case PackageVersionMultiProperty::PackageFamilyName:
                    for (std::string pfn : m_versionInfo.PackageFamilyNames)
                    {
                        result.emplace_back(Utility::LocIndString{ pfn });
                    }
                    break;
                case PackageVersionMultiProperty::ProductCode:
                    for (std::string productCode : m_versionInfo.ProductCodes)
                    {
                        result.emplace_back(Utility::LocIndString{ productCode });
                    }
                    break;
                case PackageVersionMultiProperty::UpgradeCode:
                    for (std::string upgradeCode : m_versionInfo.UpgradeCodes)
                    {
                        result.emplace_back(Utility::LocIndString{ upgradeCode });
                    }
                    break;
                case PackageVersionMultiProperty::Name:
                    if (m_versionInfo.Manifest)
                    {
                        for (auto name : m_versionInfo.Manifest->GetPackageNames())
                        {
                            result.emplace_back(std::move(name));
                        }
                    }
                    else
                    {
                        result.emplace_back(m_package->PackageInfo().PackageName);
                    }
                    break;
                case PackageVersionMultiProperty::Publisher:
                    if (m_versionInfo.Manifest)
                    {
                        for (auto publisher : m_versionInfo.Manifest->GetPublishers())
                        {
                            result.emplace_back(std::move(publisher));
                        }
                    }
                    else
                    {
                        result.emplace_back(m_package->PackageInfo().Publisher);
                    }
                    break;
                case PackageVersionMultiProperty::Locale:
                    if (m_versionInfo.Manifest)
                    {
                        result.emplace_back(m_versionInfo.Manifest->DefaultLocalization.Locale);
                        for (const auto& loc : m_versionInfo.Manifest->Localizations)
                        {
                            result.emplace_back(loc.Locale);
                        }
                    }
                    break;
                }

                return result;
            }

            Manifest::Manifest GetManifest() override
            {
                AICLI_LOG(Repo, Verbose, << "Getting manifest");

                if (m_versionInfo.Manifest)
                {
                    return m_versionInfo.Manifest.value();
                }

                if (m_package->HandleSingleUnknownVersion(m_versionInfo) &&
                    m_versionInfo.Manifest)
                {
                    return m_versionInfo.Manifest.value();
                }

                std::optional<Manifest::Manifest> manifest = GetReferenceSource()->GetRestClient().GetManifestByVersion(
                    m_package->PackageInfo().PackageIdentifier, m_versionInfo.VersionAndChannel.GetVersion().ToString(), m_versionInfo.VersionAndChannel.GetChannel().ToString());

                if (!manifest)
                {
                    AICLI_LOG(Repo, Verbose, << "Valid manifest not found for package: " << m_package->PackageInfo().PackageIdentifier);
                    return {};
                }
                
                m_versionInfo.Manifest = std::move(manifest.value());
                return m_versionInfo.Manifest.value();
            }

            Source GetSource() const override
            {
                return Source{ GetReferenceSource() };
            }

            IPackageVersion::Metadata GetMetadata() const override
            {
                IPackageVersion::Metadata result;
                return result;
            }

        private:
            std::shared_ptr<RestPackage> m_package;
            IRestClient::VersionInfo m_versionInfo;
        };

        std::shared_ptr<IPackageVersion> RestPackage::GetVersion(const PackageVersionKey& versionKey) const
        {
            std::shared_ptr<RestSource> source = GetReferenceSource();
            std::scoped_lock versionsLock{ m_packageVersionsLock };

            // Ensure that this key targets this (or any) source
            if (!versionKey.SourceId.empty() && versionKey.SourceId != source->GetIdentifier())
            {
                return {};
            }

            std::shared_ptr<IPackageVersion> packageVersion;
            if (!versionKey.Version.empty() && !versionKey.Channel.empty())
            {
                for (const auto& versionInfo : m_package.Versions)
                {
                    if (CaseInsensitiveEquals(versionInfo.VersionAndChannel.GetVersion().ToString(), versionKey.Version)
                        && CaseInsensitiveEquals(versionInfo.VersionAndChannel.GetChannel().ToString(), versionKey.Channel))
                    {
                        packageVersion = std::make_shared<PackageVersion>(source, NonConstSharedFromThis(), versionInfo);
                        break;
                    }
                }
            }
            else if (versionKey.Version.empty() && versionKey.Channel.empty())
            {
                packageVersion = GetLatestVersionInternal();
            }
            else if (versionKey.Version.empty())
            {
                for (const auto& versionInfo : m_package.Versions)
                {
                    if (CaseInsensitiveEquals(versionInfo.VersionAndChannel.GetChannel().ToString(), versionKey.Channel))
                    {
                        packageVersion = std::make_shared<PackageVersion>(source, NonConstSharedFromThis(), versionInfo);
                        break;
                    }
                }
            }
            else if (versionKey.Channel.empty())
            {
                for (const auto& versionInfo : m_package.Versions)
                {
                    if (CaseInsensitiveEquals(versionInfo.VersionAndChannel.GetVersion().ToString(), versionKey.Version))
                    {
                        packageVersion = std::make_shared<PackageVersion>(source, NonConstSharedFromThis(), versionInfo);
                        break;
                    }
                }
            }

            return packageVersion;
        }

        std::shared_ptr<IPackageVersion> RestPackage::GetLatestVersionInternal() const
        {
            return std::make_shared<PackageVersion>(GetReferenceSource(), NonConstSharedFromThis(), m_package.Versions.front());
        }
    }

    RestSource::RestSource(const SourceDetails& details, SourceInformation information, RestClient&& restClient)
        : m_details(details), m_information(std::move(information)), m_restClient(std::move(restClient))
    {
    }

    const std::string& RestSource::GetIdentifier() const
    {
        return m_details.Identifier;
    }

    const SourceDetails& RestSource::GetDetails() const
    {
        return m_details;
    }

    SourceInformation RestSource::GetInformation() const
    {
        return m_information;
    }

    bool RestSource::QueryFeatureFlag(SourceFeatureFlag flag) const
    {
        switch (flag)
        {
        case SourceFeatureFlag::ManifestMayContainAdditionalSystemReferenceStrings:
            return true;
        }

        return false;
    }

    SearchResult RestSource::Search(const SearchRequest& request) const
    {
        IRestClient::SearchResult results = m_restClient.Search(request);
        SearchResult searchResult;

        std::shared_ptr<RestSource> sharedThis = NonConstSharedFromThis();
        for (auto& result : results.Matches)
        {
            std::shared_ptr<ICompositePackage> package = std::make_shared<RestPackage>(sharedThis, std::move(result));

            // TODO: Improve to use Package match filter to return relevant search results.
            PackageMatchFilter packageFilter{ {}, {}, {} };

            searchResult.Matches.emplace_back(std::move(package), std::move(packageFilter));
        }

        searchResult.Truncated = results.Truncated;

        return searchResult;
    }

    void* RestSource::CastTo(ISourceType type)
    {
        if (type == SourceType)
        {
            return this;
        }

        return nullptr;
    }

    const RestClient& RestSource::GetRestClient() const
    {
        return m_restClient;
    }

    bool RestSource::IsSame(const RestSource* other) const
    {
        return (other && GetIdentifier() == other->GetIdentifier());
    }

    std::shared_ptr<RestSource> RestSource::NonConstSharedFromThis() const
    {
        return const_cast<RestSource*>(this)->shared_from_this();
    }
}
