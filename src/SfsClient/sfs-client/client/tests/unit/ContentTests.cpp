// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentUtil.h"
#include "sfsclient/Content.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[ContentTests] " __VA_ARGS__)
#define TEST_SCENARIO(...) TEST_CASE("[ContentTests] Scenario: " __VA_ARGS__)

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

std::unique_ptr<Content> GetContent(const std::string& contentNameSpace,
                                    const std::string& contentName,
                                    const std::string& contentVersion,
                                    const std::vector<File>& files)
{
    std::unique_ptr<Content> content;
    REQUIRE(Content::Make(contentNameSpace, contentName, contentVersion, files, content) == Result::Success);
    REQUIRE(content != nullptr);
    return content;
};
} // namespace

TEST_SCENARIO("Testing Content::Make()")
{
    GIVEN("Elements that make up a content")
    {
        const std::string contentNameSpace{"myNameSpace"};
        const std::string contentName{"myName"};
        const std::string contentVersion{"myVersion"};

        std::unique_ptr<File> file1 = GetFile("fileId1", "url1", 1 /*sizeInBytes*/, {{HashType::Sha1, "sha1"}});
        std::unique_ptr<File> file2 = GetFile("fileId2", "url2", 1 /*sizeInBytes*/, {{HashType::Sha256, "sha256"}});

        std::vector<File> files;
        files.push_back(std::move(*file1));
        files.push_back(std::move(*file2));

        // Getting raw pointers to check they don't match after copy but match after move
        std::vector<File*> filePointers;
        for (size_t i = 0; i < files.size(); ++i)
        {
            filePointers.push_back(&files[i]);
        }

        WHEN("A Content is created by copying the parameters")
        {
            std::unique_ptr<Content> copiedContent;
            REQUIRE(Content::Make(contentNameSpace, contentName, contentVersion, files, copiedContent) ==
                    Result::Success);
            REQUIRE(copiedContent != nullptr);

            THEN("The content elements are copies")
            {
                CHECK(contentNameSpace == copiedContent->GetContentId().GetNameSpace());
                CHECK(contentName == copiedContent->GetContentId().GetName());
                CHECK(contentVersion == copiedContent->GetContentId().GetVersion());

                // Files were cloned, so the pointers are different, but the contents should be similar
                REQUIRE(files.size() == copiedContent->GetFiles().size());
                REQUIRE(filePointers.size() == copiedContent->GetFiles().size());
                for (size_t i = 0; i < files.size(); ++i)
                {
                    // Checking ptrs
                    REQUIRE(filePointers[i] != &copiedContent->GetFiles()[i]);
                    REQUIRE(&files[i] != &copiedContent->GetFiles()[i]);

                    // Checking contents
                    CHECK((files[i] == copiedContent->GetFiles()[i]));
                }
            }

            AND_THEN("Using the Make() that moves the file parameter really moves the parameter")
            {
                std::unique_ptr<Content> movedContent;
                REQUIRE(Content::Make(contentNameSpace, contentName, contentVersion, std::move(files), movedContent) ==
                        Result::Success);
                REQUIRE(movedContent != nullptr);

                // Checking contents
                CHECK((*copiedContent == *movedContent));

                // Checking underlying pointers are the same since they were moved
                REQUIRE(filePointers.size() == movedContent->GetFiles().size());
                REQUIRE(copiedContent->GetFiles().size() == movedContent->GetFiles().size());
                for (size_t i = 0; i < filePointers.size(); ++i)
                {
                    REQUIRE(&copiedContent->GetFiles()[i] != &movedContent->GetFiles()[i]);
                    REQUIRE(filePointers[i] == &movedContent->GetFiles()[i]);
                }
            }
        }
    }
}

TEST("Testing Content equality operators")
{
    const std::string contentNameSpace{"myNameSpace"};
    const std::string contentName{"myName"};
    const std::string contentVersion{"myVersion"};

    std::unique_ptr<File> file = GetFile("fileId", "url", 1 /*sizeInBytes*/, {{HashType::Sha1, "sha1"}});

    std::unique_ptr<File> clonedFile;
    REQUIRE(File::Make(file->GetFileId(), file->GetUrl(), file->GetSizeInBytes(), file->GetHashes(), clonedFile) ==
            Result::Success);

    std::vector<File> files;
    files.push_back(std::move(*file));

    std::vector<File> clonedFiles;
    clonedFiles.push_back(std::move(*clonedFile));

    const std::unique_ptr<Content> content = GetContent(contentNameSpace, contentName, contentVersion, files);

    SECTION("Equal")
    {
        auto CompareContentEqual = [&content](const std::unique_ptr<Content>& sameContent) {
            REQUIRE((*content == *sameContent));
            REQUIRE_FALSE((*content != *sameContent));
        };

        CompareContentEqual(GetContent(contentNameSpace, contentName, contentVersion, files));
        CompareContentEqual(GetContent(contentNameSpace, contentName, contentVersion, clonedFiles));
    }

    SECTION("Not equal")
    {
        auto CompareContentNotEqual = [&content](const std::unique_ptr<Content>& otherContent) {
            REQUIRE((*content != *otherContent));
            REQUIRE_FALSE((*content == *otherContent));
        };

        CompareContentNotEqual(GetContent("", contentName, contentVersion, files));
        CompareContentNotEqual(GetContent(contentNameSpace, "", contentVersion, files));
        CompareContentNotEqual(GetContent(contentNameSpace, contentName, "", files));
        CompareContentNotEqual(GetContent(contentNameSpace, contentName, contentVersion, {}));
        CompareContentNotEqual(GetContent("", "", "", {}));
        CompareContentNotEqual(GetContent("MYNAMESPACE", contentName, contentVersion, files));
        CompareContentNotEqual(GetContent(contentNameSpace, "MYNAME", contentVersion, files));
        CompareContentNotEqual(GetContent(contentNameSpace, contentName, "MYVERSION", files));
    }
}
