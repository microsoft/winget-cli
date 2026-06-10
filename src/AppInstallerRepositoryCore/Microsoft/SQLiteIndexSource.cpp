// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "Microsoft/SQLiteIndexSourceV1.h"
#include "Microsoft/SQLiteIndexSourceV2.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include <winget/ManifestYamlParser.h>
#include <winget/PackageVersionDataManifest.h>

using namespace AppInstaller::Utility;


namespace AppInstaller::Repository::Microsoft
{
    namespace details
    {
        SourceReference::SourceReference(const std::shared_ptr<SQLiteIndexSource>& source) :
            m_source(source) {}

        std::shared_ptr<SQLiteIndexSource> SourceReference::GetReferenceSource() const
        {
            std::shared_ptr<SQLiteIndexSource> source = m_source.lock();
            THROW_HR_IF(E_NOT_VALID_STATE, !source);
            return source;
        }
    }

    SQLiteIndexSource::SQLiteIndexSource(
        const SourceDetails& details,
        SQLiteIndex&& index,
        bool isInstalledSource,
        bool requireManifestHash) :
        m_details(details), m_isInstalled(isInstalledSource), m_index(std::move(index)), m_requireManifestHash(requireManifestHash)
    {
        std::vector<std::string> cacheSources;
        cacheSources.push_back(m_details.Arg);
        if (!m_details.AlternateArg.empty())
        {
            cacheSources.push_back(m_details.AlternateArg);
        }

        switch (m_index.GetVersion().MajorVersion)
        {
        case 1:
            m_manifestCache = std::make_shared<Caching::FileCache>(Caching::FileCache::Type::IndexV1_Manifest, m_details.Identifier, std::move(cacheSources));
            break;
        case 2:
            m_manifestCache = std::make_shared<Caching::FileCache>(Caching::FileCache::Type::IndexV2_Manifest, m_details.Identifier, cacheSources);
            m_packageVersionDataCache = std::make_shared<Caching::FileCache>(Caching::FileCache::Type::IndexV2_PackageVersionData, m_details.Identifier, std::move(cacheSources));
            break;
        default:
            THROW_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    const SourceDetails& SQLiteIndexSource::GetDetails() const
    {
        return m_details;
    }

    const std::string& SQLiteIndexSource::GetIdentifier() const
    {
        return m_details.Identifier;
    }

    SearchResult SQLiteIndexSource::Search(const SearchRequest& request) const
    {
        auto indexResults = m_index.Search(request);

        SearchResult result;
        std::shared_ptr<SQLiteIndexSource> sharedThis = NonConstSharedFromThis();
        uint32_t majorVersion = m_index.GetVersion().MajorVersion;

        for (auto& indexResult : indexResults.Matches)
        {
            std::shared_ptr<ICompositePackage> package;

            switch (majorVersion)
            {
            case 1:
                package = std::make_shared<details::V1::SQLitePackage>(sharedThis, indexResult.first, m_manifestCache, m_isInstalled);
                break;
            case 2:
                package = std::make_shared<details::V2::SQLitePackage>(sharedThis, indexResult.first, m_manifestCache, m_packageVersionDataCache, m_isInstalled);
                break;
            default:
                THROW_WIN32(ERROR_NOT_SUPPORTED);
            }

            result.Matches.emplace_back(
                std::move(package),
                std::move(indexResult.second));
        }

        result.Truncated = indexResults.Truncated;
        return result;
    }

    void* SQLiteIndexSource::CastTo(ISourceType type)
    {
        if (type == SourceType)
        {
            return this;
        }

        return nullptr;
    }

    bool SQLiteIndexSource::IsSame(const SQLiteIndexSource* other) const
    {
        return (other && GetIdentifier() == other->GetIdentifier());
    }

    std::shared_ptr<SQLiteIndexSource> SQLiteIndexSource::NonConstSharedFromThis() const
    {
        return const_cast<SQLiteIndexSource*>(this)->shared_from_this();
    }

    SQLiteIndexWriteableSource::SQLiteIndexWriteableSource(const SourceDetails& details, SQLiteIndex&& index, bool isInstalledSource) :
        SQLiteIndexSource(details, std::move(index), isInstalledSource)
    {
    }

    void* SQLiteIndexWriteableSource::CastTo(ISourceType type)
    {
        if (type == ISourceType::IMutablePackageSource)
        {
            return static_cast<IMutablePackageSource*>(this);
        }

        return SQLiteIndexSource::CastTo(type);
    }

    void SQLiteIndexWriteableSource::AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        m_index.AddManifest(manifest, relativePath);
    }
    
    void SQLiteIndexWriteableSource::RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        m_index.RemoveManifest(manifest, relativePath);
    }
}
