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
    struct UpstreamFileInfo
    {
        TestDataFile OriginalFile;
        std::filesystem::path Offset;
        std::filesystem::path UpstreamPath;
        std::vector<uint8_t> Contents;
        SHA256::HashBuffer ContentHash;
    };

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

    UpstreamFileInfo PrepareUpstreamFile(const std::filesystem::path& testDataFile, const std::filesystem::path& offset = {}, size_t index = 0)
    {
        UpstreamFileInfo result{ testDataFile };

        auto dataFilePath = result.OriginalFile.GetPath();

        result.Offset = offset.empty() ? dataFilePath.filename() : offset;
        result.UpstreamPath = UpstreamSources[index].GetPath() / result.Offset;

        std::filesystem::copy_file(dataFilePath, result.UpstreamPath);

        std::ifstream fileStream{ dataFilePath, std::ios_base::in | std::ios_base::binary };
        result.Contents = ReadEntireStreamAsByteArray(fileStream);

        result.ContentHash = SHA256::ComputeHash(result.Contents);

        return result;
    }

    std::filesystem::path GetCacheFilePath(const UpstreamFileInfo& upstreamFileInfo)
    {
        std::filesystem::path result = CachePtr->GetDetails().GetCachePath() / upstreamFileInfo.Offset;
        std::filesystem::create_directories(result.parent_path());
        return result;
    }

    std::unique_ptr<std::istream> GetFile(const UpstreamFileInfo& upstreamFileInfo)
    {
        return CachePtr->GetFile(upstreamFileInfo.Offset, upstreamFileInfo.ContentHash);
    }

    void RequireCachedFile(const UpstreamFileInfo& upstreamFileInfo)
    {
        std::filesystem::path cachedFilePath = GetCacheFilePath(upstreamFileInfo);
        REQUIRE(std::filesystem::is_regular_file(cachedFilePath));
        REQUIRE(SHA256::AreEqual(upstreamFileInfo.ContentHash, SHA256::ComputeHashFromFile(cachedFilePath)));
    }

    std::unique_ptr<FileCache> CachePtr;
    std::vector<TempDirectory> UpstreamSources;
};

TEST_CASE("FileCache_TypeLocationsDiffer", "[file_cache]")
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

TEST_CASE("FileCache_TypeLocationsSame", "[file_cache]")
{
    std::string identifier = "identifier";
    std::string source = "source";

    REQUIRE(FileCache(FileCache::Type::IndexV1_Manifest, identifier, {}).GetDetails().GetCachePath() == FileCache(FileCache::Type::IndexV1_Manifest, identifier, { source }).GetDetails().GetCachePath());
    REQUIRE(FileCache(FileCache::Type::IndexV2_Manifest, identifier, {}).GetDetails().GetCachePath() == FileCache(FileCache::Type::IndexV2_Manifest, identifier, { source }).GetDetails().GetCachePath());
    REQUIRE(FileCache(FileCache::Type::IndexV2_PackageVersionData, identifier, {}).GetDetails().GetCachePath() == FileCache(FileCache::Type::IndexV2_PackageVersionData, identifier, { source }).GetDetails().GetCachePath());
}

TEST_CASE("FileCache_NoCachedFile", "[file_cache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    auto sourceFile = testFileCache.PrepareUpstreamFile("Manifest-Good-SystemReferenceComplex.yaml");

    auto cachedStream = testFileCache.GetFile(sourceFile);

    REQUIRE(cachedStream);
    REQUIRE(SHA256::AreEqual(sourceFile.ContentHash, SHA256::ComputeHash(ReadEntireStreamAsByteArray(*cachedStream))));

    testFileCache.RequireCachedFile(sourceFile);
}

TEST_CASE("FileCache_CachedFileIsDirectory", "[file_cache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    auto sourceFile = testFileCache.PrepareUpstreamFile("Manifest-Good.yaml");
    std::filesystem::create_directories(testFileCache.GetCacheFilePath(sourceFile));

    auto cachedStream = testFileCache.GetFile(sourceFile);

    REQUIRE(cachedStream);
    REQUIRE(SHA256::AreEqual(sourceFile.ContentHash, SHA256::ComputeHash(ReadEntireStreamAsByteArray(*cachedStream))));

    testFileCache.RequireCachedFile(sourceFile);
}

TEST_CASE("FileCache_CachedFileGoodHash", "[file_cache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    auto sourceFile = testFileCache.PrepareUpstreamFile("InstallFlowTest_MSStore.yaml");
    std::filesystem::copy_file(sourceFile.OriginalFile, testFileCache.GetCacheFilePath(sourceFile));

    auto cachedStream = testFileCache.GetFile(sourceFile);

    REQUIRE(cachedStream);
    REQUIRE(SHA256::AreEqual(sourceFile.ContentHash, SHA256::ComputeHash(ReadEntireStreamAsByteArray(*cachedStream))));

    testFileCache.RequireCachedFile(sourceFile);
}

TEST_CASE("FileCache_CachedFileBadHash", "[file_cache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    auto sourceFile = testFileCache.PrepareUpstreamFile("ManifestV1-MultiFile-Version.yaml");
    std::filesystem::copy_file(TestDataFile("Manifest-Bad-ProductCodeOnMSIX.yaml"), testFileCache.GetCacheFilePath(sourceFile));

    auto cachedStream = testFileCache.GetFile(sourceFile);

    REQUIRE(cachedStream);
    REQUIRE(SHA256::AreEqual(sourceFile.ContentHash, SHA256::ComputeHash(ReadEntireStreamAsByteArray(*cachedStream))));

    testFileCache.RequireCachedFile(sourceFile);
}

TEST_CASE("FileCache_CachedFileLockedExclusive", "[file_cache]")
{
    TestFileCache testFileCache;
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    auto sourceFile = testFileCache.PrepareUpstreamFile("ManifestV1-MultiFile-Installer.yaml");
    TestDataFile wrongFileOriginal = TestDataFile("Manifest-Bad-InvalidLocale.yaml");
    std::filesystem::path wrongFilePath = testFileCache.GetCacheFilePath(sourceFile);
    std::filesystem::copy_file(wrongFileOriginal, wrongFilePath);
    wil::unique_handle exclusiveFileHandle{ CreateFileW(wrongFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) };

    auto cachedStream = testFileCache.GetFile(sourceFile);

    REQUIRE(cachedStream);
    REQUIRE(SHA256::AreEqual(sourceFile.ContentHash, SHA256::ComputeHash(ReadEntireStreamAsByteArray(*cachedStream))));
}

TEST_CASE("FileCache_FirstUpstreamDoesNotHaveFile", "[file_cache]")
{
    TestFileCache testFileCache({}, 2);
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    auto sourceFile = testFileCache.PrepareUpstreamFile("Manifest-Good-MultiLocale.yaml", {}, 1);

    auto cachedStream = testFileCache.GetFile(sourceFile);

    REQUIRE(cachedStream);
    REQUIRE(SHA256::AreEqual(sourceFile.ContentHash, SHA256::ComputeHash(ReadEntireStreamAsByteArray(*cachedStream))));

    testFileCache.RequireCachedFile(sourceFile);
}

TEST_CASE("FileCache_FirstUpstreamHasBadHash", "[file_cache]")
{
    TestFileCache testFileCache({}, 2);
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    auto badFile = testFileCache.PrepareUpstreamFile("Manifest-Bad-VersionMissing.yaml", {}, 0);
    auto sourceFile = testFileCache.PrepareUpstreamFile("Manifest-Good-MultiLocale.yaml", {}, 1);

    auto cachedStream = testFileCache.GetFile(sourceFile);

    REQUIRE(cachedStream);
    REQUIRE(SHA256::AreEqual(sourceFile.ContentHash, SHA256::ComputeHash(ReadEntireStreamAsByteArray(*cachedStream))));

    testFileCache.RequireCachedFile(sourceFile);
}

TEST_CASE("FileCache_NoUpstreamSources", "[file_cache]")
{
    TestFileCache testFileCache("", 0);
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    REQUIRE_THROWS_HR(testFileCache->GetFile("any_file", SHA256::ComputeHash("garbage")), E_NOT_SET);
}

TEST_CASE("FileCache_PathTooLong", "[file_cache]")
{
    TestFileCache testFileCache(std::string(260, 'a'));
    INFO("Cache location: " << testFileCache->GetDetails().GetCachePath().u8string());

    auto sourceFile = testFileCache.PrepareUpstreamFile("Manifest-Good-SystemReferenceComplex.yaml");

    auto cachedStream = testFileCache.GetFile(sourceFile);

    REQUIRE(cachedStream);
    REQUIRE(SHA256::AreEqual(sourceFile.ContentHash, SHA256::ComputeHash(ReadEntireStreamAsByteArray(*cachedStream))));
}
