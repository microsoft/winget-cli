// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentUtil.h"
#include "sfsclient/File.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[FileTests] " __VA_ARGS__)
#define TEST_SCENARIO(...) TEST_CASE("[FileTests] Scenario: " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::details::contentutil;

namespace
{
std::unique_ptr<File> GetFile(const std::string& fileId,
                              const std::string& url,
                              uint64_t sizeInBytes,
                              const std::unordered_map<HashType, std::string>& hashes)
{
    std::unique_ptr<File> file;
    REQUIRE(File::Make(fileId, url, sizeInBytes, hashes, file) == Result::Success);
    REQUIRE(file != nullptr);
    return file;
};
} // namespace

TEST("Testing File::Make()")
{
    const std::string fileId{"myFileId"};
    const std::string url{"myUrl"};
    const uint64_t sizeInBytes{1234};
    const std::unordered_map<HashType, std::string> hashes{{HashType::Sha1, "mySha1"}, {HashType::Sha256, "mySha256"}};

    const std::unique_ptr<File> file = GetFile(fileId, url, sizeInBytes, hashes);

    CHECK(fileId == file->GetFileId());
    CHECK(url == file->GetUrl());
    CHECK(sizeInBytes == file->GetSizeInBytes());
    CHECK(hashes == file->GetHashes());

    SECTION("Testing File equality operators")
    {
        SECTION("Equal")
        {
            auto CompareFileEqual = [&file](const std::unique_ptr<File>& sameFile) {
                REQUIRE((*file == *sameFile));
                REQUIRE_FALSE((*file != *sameFile));
            };

            CompareFileEqual(GetFile(fileId, url, sizeInBytes, hashes));
        }

        SECTION("Not equal")
        {
            auto CompareFileNotEqual = [&file](const std::unique_ptr<File>& otherFile) {
                REQUIRE((*file != *otherFile));
                REQUIRE_FALSE((*file == *otherFile));
            };

            CompareFileNotEqual(GetFile("", url, sizeInBytes, hashes));
            CompareFileNotEqual(GetFile(fileId, "", sizeInBytes, hashes));
            CompareFileNotEqual(GetFile(fileId, url, 0, hashes));
            CompareFileNotEqual(GetFile(fileId, url, sizeInBytes, {}));
            CompareFileNotEqual(GetFile("", "", 0, {}));
            CompareFileNotEqual(GetFile("MYFILEID", url, sizeInBytes, hashes));
            CompareFileNotEqual(GetFile(fileId, "MYURL", sizeInBytes, hashes));
        }
    }
}
