// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentUtil.h"
#include "sfsclient/AppFile.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[AppFileTests] " __VA_ARGS__)
#define TEST_SCENARIO(...) TEST_CASE("[AppFileTests] Scenario: " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::details::contentutil;

namespace
{
std::unique_ptr<AppFile> GetAppFile(const std::string& fileId,
                                    const std::string& url,
                                    uint64_t sizeInBytes,
                                    const std::unordered_map<HashType, std::string>& hashes,
                                    std::vector<Architecture> architectures,
                                    std::vector<std::string> platformApplicabilityForPackage,
                                    std::string fileMoniker)
{
    std::unique_ptr<AppFile> file;
    REQUIRE(AppFile::Make(fileId,
                          url,
                          sizeInBytes,
                          hashes,
                          architectures,
                          platformApplicabilityForPackage,
                          fileMoniker,
                          file) == Result::Success);
    REQUIRE(file != nullptr);
    return file;
}
} // namespace

TEST("Testing AppFile::Make()")
{
    const std::string fileId{"myFileId"};
    const std::string url{"myUrl"};
    const uint64_t sizeInBytes{1234};
    const std::unordered_map<HashType, std::string> hashes{{HashType::Sha1, "mySha1"}, {HashType::Sha256, "mySha256"}};
    const std::vector<Architecture> architectures{Architecture::Amd64};
    const std::vector<std::string> platformApplicabilityForPackage{"myPlatformApplicabilityForPackage"};
    const std::string fileMoniker{"myFileMoniker"};

    const std::unique_ptr<AppFile> file =
        GetAppFile(fileId, url, sizeInBytes, hashes, architectures, platformApplicabilityForPackage, fileMoniker);

    CHECK(fileId == file->GetFileId());
    CHECK(url == file->GetUrl());
    CHECK(sizeInBytes == file->GetSizeInBytes());
    CHECK(hashes == file->GetHashes());

    SECTION("Testing File equality operators")
    {
        SECTION("Equal")
        {
            auto CompareFileEqual = [&file](const std::unique_ptr<AppFile>& sameFile) {
                REQUIRE((*file == *sameFile));
                REQUIRE_FALSE((*file != *sameFile));
            };

            CompareFileEqual(GetAppFile(fileId,
                                        url,
                                        sizeInBytes,
                                        hashes,
                                        architectures,
                                        platformApplicabilityForPackage,
                                        fileMoniker));
        }

        SECTION("Not equal")
        {
            auto CompareFileNotEqual = [&file](const std::unique_ptr<AppFile>& otherFile) {
                REQUIRE((*file != *otherFile));
                REQUIRE_FALSE((*file == *otherFile));
            };

            CompareFileNotEqual(
                GetAppFile("", url, sizeInBytes, hashes, architectures, platformApplicabilityForPackage, fileMoniker));
            CompareFileNotEqual(GetAppFile(fileId,
                                           "",
                                           sizeInBytes,
                                           hashes,
                                           architectures,
                                           platformApplicabilityForPackage,
                                           fileMoniker));
            CompareFileNotEqual(
                GetAppFile(fileId, url, 0, hashes, architectures, platformApplicabilityForPackage, fileMoniker));
            CompareFileNotEqual(
                GetAppFile(fileId, url, sizeInBytes, {}, architectures, platformApplicabilityForPackage, fileMoniker));
            CompareFileNotEqual(
                GetAppFile(fileId, url, sizeInBytes, hashes, {}, platformApplicabilityForPackage, fileMoniker));
            CompareFileNotEqual(GetAppFile(fileId, url, sizeInBytes, hashes, architectures, {}, fileMoniker));
            CompareFileNotEqual(GetAppFile(fileId, url, sizeInBytes, hashes, {}, platformApplicabilityForPackage, {}));
            CompareFileNotEqual(GetAppFile("", "", 0, {}, architectures, platformApplicabilityForPackage, fileMoniker));
            CompareFileNotEqual(GetAppFile("MYFILEID",
                                           url,
                                           sizeInBytes,
                                           hashes,
                                           architectures,
                                           platformApplicabilityForPackage,
                                           fileMoniker));
            CompareFileNotEqual(GetAppFile(fileId,
                                           "MYURL",
                                           sizeInBytes,
                                           hashes,
                                           architectures,
                                           platformApplicabilityForPackage,
                                           fileMoniker));
        }
    }
}
