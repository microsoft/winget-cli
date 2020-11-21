// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include <winget/ManifestYamlParser.h>


namespace AppInstaller::Repository::Microsoft
{
    using namespace Utility;

    namespace
    {
        // The base for the package objects.
        struct SourceReference
        {
            SourceReference(const std::shared_ptr<const SQLiteIndexSource>& source) :
                m_source(source) {}

        protected:
            std::shared_ptr<const SQLiteIndexSource> GetReferenceSource() const
            {
                std::shared_ptr<const SQLiteIndexSource> source = m_source.lock();
                THROW_HR_IF(E_NOT_VALID_STATE, !source);
                return source;
            }

        private:
            std::weak_ptr<const SQLiteIndexSource> m_source;
        };

        // The IPackageVersion impl for SQLiteIndexSource.
        struct PackageVersion : public SourceReference, public IPackageVersion
        {
            PackageVersion(const std::shared_ptr<const SQLiteIndexSource>& source, SQLiteIndex::IdType manifestId) :
                SourceReference(source), m_manifestId(manifestId) {}

            // Inherited via IPackageVersion
            Utility::LocIndString GetProperty(PackageVersionProperty property) const override
            {
                switch (property)
                {
                case PackageVersionProperty::SourceIdentifier:
                    return LocIndString{ GetReferenceSource()->GetIdentifier() };
                case PackageVersionProperty::SourceName:
                    return LocIndString{ GetReferenceSource()->GetDetails().Name };
                default:
                    // Values coming from the index will always be localized/independent.
                    return LocIndString{ GetReferenceSource()->GetIndex().GetPropertyByManifestId(m_manifestId, property).value() };
                }
            }

            std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
            {
                std::vector<Utility::LocIndString> result;

                for (auto&& value : GetReferenceSource()->GetIndex().GetMultiPropertyByManifestId(m_manifestId, property))
                {
                    // Values coming from the index will always be localized/independent.
                    result.emplace_back(std::move(value));
                }

                return result;
            }

            Manifest::Manifest GetManifest() const override
            {
                std::shared_ptr<const SQLiteIndexSource> source = GetReferenceSource();
                std::optional<std::string> relativePathOpt = source->GetIndex().GetPropertyByManifestId(m_manifestId, PackageVersionProperty::RelativePath);
                THROW_HR_IF(E_NOT_SET, !relativePathOpt);
                return GetManifestFromArgAndRelativePath(source->GetDetails().Arg, relativePathOpt.value());
            }

            std::shared_ptr<const ISource> GetSource() const override
            {
                return GetReferenceSource();
            }

            IPackageVersion::Metadata GetMetadata() const override
            {
                auto metadata = GetReferenceSource()->GetIndex().GetMetadataByManifestId(m_manifestId);

                IPackageVersion::Metadata result;
                for (auto&& data : metadata)
                {
                    result.emplace(std::move(data));
                }

                return result;
            }

        private:
            static Manifest::Manifest GetManifestFromArgAndRelativePath(const std::string& arg, const std::string& relativePath)
            {
                std::string fullPath = arg;
                if (fullPath.back() != '/')
                {
                    fullPath += '/';
                }
                fullPath += relativePath;

                if (Utility::IsUrlRemote(fullPath))
                {
                    std::ostringstream manifestStream;

                    AICLI_LOG(Repo, Info, << "Downloading manifest");
                    ProgressCallback emptyCallback;

                    const int MaxRetryCount = 2;
                    for (int retryCount = 0; retryCount < MaxRetryCount; ++retryCount)
                    {
                        bool success = false;
                        try
                        {
                            (void)Utility::DownloadToStream(fullPath, manifestStream, emptyCallback);
                            success = true;
                        }
                        catch (...)
                        {
                            if (retryCount < MaxRetryCount - 1)
                            {
                                AICLI_LOG(Repo, Info, << "Downloading manifest failed, waiting a bit and retrying: " << fullPath);
                                Sleep(500);
                            }
                            else
                            {
                                throw;
                            }
                        }

                        if (success)
                        {
                            break;
                        }
                    }

                    std::string manifestContents = manifestStream.str();
                    AICLI_LOG(Repo, Verbose, << "Manifest contents: " << manifestContents);

                    return Manifest::YamlParser::Create(manifestContents);
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "Opening manifest from local file: " << fullPath);
                    return Manifest::YamlParser::CreateFromPath(fullPath);
                }
            }

            SQLiteIndex::IdType m_manifestId;
        };

        // The base for IPackage implementations here.
        struct PackageBase : public SourceReference
        {
            PackageBase(const std::shared_ptr<const SQLiteIndexSource>& source, SQLiteIndex::IdType idId) :
                SourceReference(source), m_idId(idId) {}

            Utility::LocIndString GetProperty(PackageProperty property) const
            {
                Utility::LocIndString result;

                std::shared_ptr<IPackageVersion> truth = GetLatestVersionInternal();
                if (truth)
                {
                    switch (property)
                    {
                    case PackageProperty::Id:
                        return truth->GetProperty(PackageVersionProperty::Id);
                    case PackageProperty::Name:
                        return truth->GetProperty(PackageVersionProperty::Name);
                    default:
                        THROW_HR(E_UNEXPECTED);
                    }
                }

                return result;
            }

        protected:
            std::shared_ptr<IPackageVersion> GetLatestVersionInternal() const
            {
                std::shared_ptr<const SQLiteIndexSource> source = GetReferenceSource();
                std::optional<SQLiteIndex::IdType> manifestId = source->GetIndex().GetManifestIdByKey(m_idId, {}, {});

                if (manifestId)
                {
                    return std::make_shared<PackageVersion>(source, manifestId.value());
                }

                return {};
            }

            SQLiteIndex::IdType m_idId;
        };

        // The IPackage impl for SQLiteIndexSource of Available packages.
        struct AvailablePackage : public PackageBase, public IPackage
        {
            using PackageBase::PackageBase;

            // Inherited via IPackage
            Utility::LocIndString GetProperty(PackageProperty property) const override
            {
                return PackageBase::GetProperty(property);
            }

            std::shared_ptr<IPackageVersion> GetInstalledVersion() const override
            {
                return {};
            }

            std::vector<PackageVersionKey> GetAvailableVersionKeys() const override
            {
                std::shared_ptr<const SQLiteIndexSource> source = GetReferenceSource();
                std::vector<Utility::VersionAndChannel> versions = source->GetIndex().GetVersionKeysById(m_idId);

                std::vector<PackageVersionKey> result;
                for (const auto& vac : versions)
                {
                    result.emplace_back(source->GetIdentifier(), vac.GetVersion().ToString(), vac.GetChannel().ToString());
                }
                return result;
            }

            std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
            {
                return GetLatestVersionInternal();
            }

            std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const override
            {
                std::shared_ptr<const SQLiteIndexSource> source = GetReferenceSource();

                // Ensure that this key targets this (or any) source
                if (!versionKey.SourceId.empty() && versionKey.SourceId != source->GetIdentifier())
                {
                    return {};
                }

                std::optional<SQLiteIndex::IdType> manifestId = source->GetIndex().GetManifestIdByKey(m_idId, versionKey.Version, versionKey.Channel);

                if (manifestId)
                {
                    return std::make_shared<PackageVersion>(source, manifestId.value());
                }

                return {};
            }

            bool IsUpdateAvailable() const override
            {
                return false;
            }
        };

        // The IPackage impl for SQLiteIndexSource of Installed packages.
        struct InstalledPackage : public PackageBase, public IPackage
        {
            using PackageBase::PackageBase;

            // Inherited via IPackage
            Utility::LocIndString GetProperty(PackageProperty property) const override
            {
                return PackageBase::GetProperty(property);
            }

            std::shared_ptr<IPackageVersion> GetInstalledVersion() const override
            {
                return GetLatestVersionInternal();
            }

            std::vector<PackageVersionKey> GetAvailableVersionKeys() const override
            {
                return {};
            }

            std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
            {
                return {};
            }

            std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey&) const override
            {
                return {};
            }

            bool IsUpdateAvailable() const override
            {
                return false;
            }
        };
    }

    SQLiteIndexSource::SQLiteIndexSource(const SourceDetails& details, std::string identifier, SQLiteIndex&& index, Synchronization::CrossProcessReaderWriteLock&& lock, bool isInstalledSource) :
        m_details(details), m_identifier(std::move(identifier)), m_lock(std::move(lock)), m_isInstalled(isInstalledSource), m_index(std::move(index))
    {
    }

    const SourceDetails& SQLiteIndexSource::GetDetails() const
    {
        return m_details;
    }

    const std::string& SQLiteIndexSource::GetIdentifier() const
    {
        return m_identifier;
    }

    SearchResult SQLiteIndexSource::Search(const SearchRequest& request) const
    {
        auto indexResults = m_index.Search(request);

        SearchResult result;
        std::shared_ptr<const SQLiteIndexSource> sharedThis = shared_from_this();
        for (auto& indexResult : indexResults.Matches)
        {
            std::unique_ptr<IPackage> package;

            if (m_isInstalled)
            {
                package = std::make_unique<InstalledPackage>(sharedThis, indexResult.first);
            }
            else
            {
                package = std::make_unique<AvailablePackage>(sharedThis, indexResult.first);
            }

            result.Matches.emplace_back(std::move(package), std::move(indexResult.second));
        }
        result.Truncated = indexResults.Truncated;
        return result;
    }
}
