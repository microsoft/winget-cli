// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>
#include <winget/FileCache.h>

using namespace AppInstaller::Caching;
using namespace AppInstaller::Utility;
using namespace TestCommon;

struct TestFileCache
{
    TestFileCache(std::string identifier = {}, size_t upstreamCount = 1)
    {
        if (identifier.empty())
        {
            identifier = ConvertToUTF8(CreateNewGuidNameWString());
        }

        std::vector<std::string> upstreamStrings;

        for (size_t i = 0; i < upstreamCount; ++i)
        {
            UpstreamSources.emplace_back("TestFileCache");
            upstreamStrings.emplace_back(UpstreamSources.back().GetPath().u8string());
        }

        CachePtr = std::make_unique<FileCache>(FileCache::Type::Tests, std::move(identifier), std::move(upstreamStrings));
    }

    FileCache& Cache() { return *CachePtr; }
    FileCache* operator->() { return CachePtr.get(); }

    std::unique_ptr<FileCache> CachePtr;
    std::vector<TempDirectory> UpstreamSources;
};

TEST_CASE("FileCache_TypeLocationsDiffer", "[filecache]")
{
    std::string identifier = "identifier";
    std::string identifier2 = "identifier2";

    REQUIRE(FileCache(FileCache::Type::IndexV1_Manifest, identifier, {}).GetDetails().GetCachePath() != FileCache(FileCache::Type::IndexV2_Manifest, identifier, {}).GetDetails().GetCachePath());
    REQUIRE(FileCache(FileCache::Type::IndexV1_Manifest, identifier, {}).GetDetails().GetCachePath() != FileCache(FileCache::Type::IndexV2_PackageVersionData, identifier, {}).GetDetails().GetCachePath());
    REQUIRE(FileCache(FileCache::Type::IndexV2_Manifest, identifier, {}).GetDetails().GetCachePath() != FileCache(FileCache::Type::IndexV2_PackageVersionData, identifier, {}).GetDetails().GetCachePath());

    REQUIRE(FileCache(FileCache::Type::IndexV1_Manifest, identifier, {}).GetDetails().GetCachePath() != FileCache(FileCache::Type::IndexV1_Manifest, identifier2, {}).GetDetails().GetCachePath());
    REQUIRE(FileCache(FileCache::Type::IndexV2_Manifest, identifier, {}).GetDetails().GetCachePath() != FileCache(FileCache::Type::IndexV2_Manifest, identifier2, {}).GetDetails().GetCachePath());
    REQUIRE(FileCache(FileCache::Type::IndexV2_PackageVersionData, identifier, {}).GetDetails().GetCachePath() != FileCache(FileCache::Type::IndexV2_PackageVersionData, identifier2, {}).GetDetails().GetCachePath());
}

TEST_CASE("FileCache_TypeLocationsSame", "[filecache]")
{
    std::string identifier = "identifier";
    std::string source = "source";

    REQUIRE(FileCache(FileCache::Type::IndexV1_Manifest, identifier, {}).GetDetails().GetCachePath() == FileCache(FileCache::Type::IndexV1_Manifest, identifier, { source }).GetDetails().GetCachePath());
    REQUIRE(FileCache(FileCache::Type::IndexV2_Manifest, identifier, {}).GetDetails().GetCachePath() == FileCache(FileCache::Type::IndexV2_Manifest, identifier, { source }).GetDetails().GetCachePath());
    REQUIRE(FileCache(FileCache::Type::IndexV2_PackageVersionData, identifier, {}).GetDetails().GetCachePath() == FileCache(FileCache::Type::IndexV2_PackageVersionData, identifier, { source }).GetDetails().GetCachePath());
}

TEST_CASE("FileCache_NoCachedFile", "[filecache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    // TODO
}

TEST_CASE("FileCache_CachedFileIsDirectory", "[filecache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    // TODO
}

TEST_CASE("FileCache_CachedFileGoodHash", "[filecache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    // TODO
}

TEST_CASE("FileCache_CachedFileBadHash", "[filecache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    // TODO
}

TEST_CASE("FileCache_CachedFileLockedExclusive", "[filecache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    // TODO
}

TEST_CASE("FileCache_FirstUpstreamDoesNotHaveFile", "[filecache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    // TODO
}

TEST_CASE("FileCache_FirstUpstreamHasBadHash", "[filecache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    // TODO
}

TEST_CASE("FileCache_NoUpstreams", "[filecache]")
{
    TestFileCache testFileCache("", 0);
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    REQUIRE_THROWS_HR(testFileCache->GetFile("any_file", SHA256::ComputeHash("garbage")), E_NOT_SET);
}
