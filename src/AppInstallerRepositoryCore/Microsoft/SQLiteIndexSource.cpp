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
            std::shared_ptr<const SQLiteIndexSource> GetSource() const
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
                case PackageVersionProperty::SourceId:
                    return LocIndString{ GetSource()->GetIdentifier() };
                default:
                    // Values coming from the index will always be localized/independent.
                    return LocIndString{ GetSource()->GetIndex().GetPropertyByManifestId(m_manifestId, property).value() };
                }
            }

            Manifest::Manifest GetManifest() const override
            {
                std::shared_ptr<const SQLiteIndexSource> source = GetSource();
                std::optional<std::string> relativePathOpt = source->GetIndex().GetPropertyByManifestId(m_manifestId, PackageVersionProperty::RelativePath);
                THROW_HR_IF(E_NOT_SET, !relativePathOpt);
                return GetManifestFromArgAndRelativePath(source->GetDetails().Arg, relativePathOpt.value());
            }

            std::map<std::string, std::string> GetInstallationMetadata() const override
            {
                return {};
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
                    (void)Utility::DownloadToStream(fullPath, manifestStream, emptyCallback);

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

        // The IPackage impl for SQLiteIndexSource.
        struct Package : public SourceReference, public IPackage
        {
            Package(const std::shared_ptr<const SQLiteIndexSource>& source, SQLiteIndex::IdType idId) :
                SourceReference(source), m_idId(idId) {}

            // Inherited via IPackage
            std::shared_ptr<IPackageVersion> GetInstalledVersion() const override
            {
                // Although an index might be the backing store for installed packages, the installed package version
                // will be selected by external business logic.
                return {};
            }

            std::vector<PackageVersionKey> GetAvailableVersionKeys() const override
            {
                std::shared_ptr<const SQLiteIndexSource> source = GetSource();
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
                // Although we could potentially increase efficiency here, this should be fine.
                std::vector<PackageVersionKey> versions = GetAvailableVersionKeys();

                if (!versions.empty())
                {
                    return GetAvailableVersion(versions[0]);
                }

                return {};
            }

            std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const override
            {
                std::shared_ptr<const SQLiteIndexSource> source = GetSource();
                THROW_HR_IF(E_INVALIDARG, !versionKey.SourceId.empty() && versionKey.SourceId != source->GetIdentifier());
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

        private:
            SQLiteIndex::IdType m_idId;
        };
    }

    SQLiteIndexSource::SQLiteIndexSource(const SourceDetails& details, std::string identifier, SQLiteIndex&& index, Synchronization::CrossProcessReaderWriteLock&& lock) :
        m_details(details), m_identifier(std::move(identifier)), m_lock(std::move(lock)), m_index(std::move(index))
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
            result.Matches.emplace_back(std::make_unique<Package>(sharedThis, indexResult.first), std::move(indexResult.second));
        }
        result.Truncated = indexResults.Truncated;
        return result;
    }
}
