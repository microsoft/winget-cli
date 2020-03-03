// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"


namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // The IApplication impl for SQLiteIndexSource.
        struct Application : public IApplication
        {
            Application(SQLiteIndexSource* source, SQLiteIndex::IdType id) :
                m_id(id), m_source(source) {}

            // Inherited via IApplication
            std::string GetId() override
            {
                return GetIndex().GetIdStringById(m_id).value();
            }

            std::string GetName() override
            {
                return GetIndex().GetNameStringById(m_id).value();
            }

            Manifest::Manifest GetManifest(std::string_view version, std::string_view channel) override
            {
                std::string relativePath = GetIndex().GetPathStringByKey(m_id, version, channel).value();

                std::string fullPath = m_source->m_details.Arg;
                if (fullPath[fullPath.length()] != '/')
                {
                    fullPath += '/';
                }
                fullPath += relativePath;

                if (Utility::IsUrlRemote(fullPath))
                {
                    std::filesystem::path tempFile = Runtime::GetPathToTemp();
                    tempFile /= PreIndexedPackageSourceFactory::Type();
                    tempFile /= relativePath;

                    AICLI_LOG(Repo, Info, << "Downloading manifest to temp file");
                    ProgressCallback emptyCallback;
                    (void)Utility::Download(fullPath, tempFile, emptyCallback);

                    return Manifest::Manifest::CreateFromPath(tempFile);
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "Opening manifest from local file: " << fullPath);
                    return Manifest::Manifest::CreateFromPath(fullPath);
                }
            }

            std::vector<std::pair<std::string, std::string>> GetVersions() override
            {
                return GetIndex().GetVersionsById(m_id);
            }

        private:
            SQLiteIndex& GetIndex()
            {
                return m_source->m_index;
            }

            // TODO: Convert to shared_ptr on OpenSource and all the downstream effects so we can be safer here
            SQLiteIndexSource* m_source;
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
        for (auto& indexResult : indexResults)
        {
            result.Matches.emplace_back(std::make_unique<Application>(this, indexResult.first), std::move(indexResult.second));
        }
        return result;
    }
}
