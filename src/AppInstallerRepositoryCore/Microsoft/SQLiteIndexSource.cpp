// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"


namespace AppInstaller::Repository::Microsoft
{
    using namespace Utility;

    namespace
    {
        // The IApplication impl for SQLiteIndexSource.
        struct Application : public IApplication
        {
            Application(std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType id) :
                m_id(id), m_source(source) {}

            // Inherited via IApplication
            LocIndString GetId() override
            {
                // Values coming from the index will always be localized/independent.
                return LocIndString{ GetSource()->GetIndex().GetIdStringById(m_id).value() };
            }

            LocIndString GetName() override
            {
                // Values coming from the index will always be localized/independent.
                return LocIndString{ GetSource()->GetIndex().GetNameStringById(m_id).value() };
            }

            std::optional<Manifest::Manifest> GetManifest(const Utility::NormalizedString& version, const Utility::NormalizedString& channel) override
            {
                std::shared_ptr<SQLiteIndexSource> source = GetSource();
                std::optional<std::string> relativePathOpt = source->GetIndex().GetPathStringByKey(m_id, version, channel);
                if (!relativePathOpt)
                {
                    return {};
                }
                std::string relativePath = relativePathOpt.value();

                std::string fullPath = source->GetDetails().Arg;
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

                    return Manifest::Manifest::Create(manifestContents);
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "Opening manifest from local file: " << fullPath);
                    return Manifest::Manifest::CreateFromPath(fullPath);
                }
            }

            std::vector<Utility::VersionAndChannel> GetVersions() override
            {
                return GetSource()->GetIndex().GetVersionsById(m_id);
            }

        private:
            std::shared_ptr<SQLiteIndexSource> GetSource()
            {
                std::shared_ptr<SQLiteIndexSource> source = m_source.lock();
                THROW_HR_IF(E_NOT_VALID_STATE, !source);
                return source;
            }

            std::weak_ptr<SQLiteIndexSource> m_source;
            SQLiteIndex::IdType m_id;
        };
    }

    SQLiteIndexSource::SQLiteIndexSource(const SourceDetails& details, SQLiteIndex&& index, Synchronization::CrossProcessReaderWriteLock&& lock) :
        m_details(details), m_lock(std::move(lock)), m_index(std::move(index))
    {
    }

    const SourceDetails& SQLiteIndexSource::GetDetails() const
    {
        return m_details;
    }

    SearchResult SQLiteIndexSource::Search(const SearchRequest& request)
    {
        auto indexResults = m_index.Search(request);

        SearchResult result;
        std::shared_ptr<SQLiteIndexSource> sharedThis = shared_from_this();
        for (auto& indexResult : indexResults.Matches)
        {
            result.Matches.emplace_back(std::make_unique<Application>(sharedThis, indexResult.first), std::move(indexResult.second));
        }
        result.Truncated = indexResults.Truncated;
        return result;
    }
}
