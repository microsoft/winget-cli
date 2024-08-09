// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentUtil.h"
#include "sfsclient/AppContent.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[AppContentTests] " __VA_ARGS__)
#define TEST_SCENARIO(...) TEST_CASE("[AppContentTests] Scenario: " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::details::contentutil;

namespace
{
std::unique_ptr<ContentId> GetContentId(const std::string& nameSpace,
                                        const std::string& name,
                                        const std::string& version)
{
    std::unique_ptr<ContentId> contentId;
    REQUIRE(ContentId::Make(nameSpace, name, version, contentId) == Result::Success);
    REQUIRE(contentId != nullptr);
    return contentId;
}

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

std::unique_ptr<AppPrerequisiteContent> GetPrerequisiteContent(const std::string& contentNameSpace,
                                                               const std::string& contentName,
                                                               const std::string& contentVersion,
                                                               const std::vector<AppFile>& files)
{
    std::unique_ptr<ContentId> contentId = GetContentId(contentNameSpace, contentName, contentVersion);
    std::vector<AppFile> clonedFiles;
    for (auto& file : files)
    {
        clonedFiles.push_back(std::move(*GetAppFile(file.GetFileId(),
                                                    file.GetUrl(),
                                                    file.GetSizeInBytes(),
                                                    file.GetHashes(),
                                                    file.GetApplicabilityDetails().GetArchitectures(),
                                                    file.GetApplicabilityDetails().GetPlatformApplicabilityForPackage(),
                                                    file.GetFileMoniker())));
    }

    std::unique_ptr<AppPrerequisiteContent> content;
    REQUIRE(AppPrerequisiteContent::Make(std::move(contentId), std::move(clonedFiles), content) == Result::Success);
    REQUIRE(content != nullptr);
    return content;
}

std::unique_ptr<AppContent> GetAppContent(const std::string& contentNameSpace,
                                          const std::string& contentName,
                                          const std::string& contentVersion,
                                          const std::string& updateId,
                                          const std::vector<AppPrerequisiteContent>& prerequisites,
                                          const std::vector<AppFile>& files)
{
    std::unique_ptr<ContentId> contentId = GetContentId(contentNameSpace, contentName, contentVersion);
    std::vector<AppPrerequisiteContent> clonedPreqs;
    for (const auto& prereq : prerequisites)
    {
        clonedPreqs.push_back(std::move(*GetPrerequisiteContent(prereq.GetContentId().GetNameSpace(),
                                                                prereq.GetContentId().GetName(),
                                                                prereq.GetContentId().GetVersion(),
                                                                prereq.GetFiles())));
    }

    std::vector<AppFile> clonedFiles;
    for (auto& file : files)
    {
        clonedFiles.push_back(std::move(*GetAppFile(file.GetFileId(),
                                                    file.GetUrl(),
                                                    file.GetSizeInBytes(),
                                                    file.GetHashes(),
                                                    file.GetApplicabilityDetails().GetArchitectures(),
                                                    file.GetApplicabilityDetails().GetPlatformApplicabilityForPackage(),
                                                    file.GetFileMoniker())));
    }

    std::unique_ptr<AppContent> appContent;
    REQUIRE(
        AppContent::Make(std::move(contentId), updateId, std::move(clonedPreqs), std::move(clonedFiles), appContent) ==
        Result::Success);
    REQUIRE(appContent != nullptr);
    return appContent;
}
} // namespace

TEST("Testing AppContent::Make()")
{
    const std::string contentNameSpace{"myNameSpace"};
    const std::string contentName{"myName"};
    const std::string contentVersion{"myVersion"};

    std::unique_ptr<AppFile> file1 = GetAppFile("fileId1",
                                                "url1",
                                                1 /*sizeInBytes*/,
                                                {{HashType::Sha1, "sha1"}},
                                                {Architecture::Amd64},
                                                {"myPlatformApplicabilityForPackage1"},
                                                "fileMoniker1");
    std::unique_ptr<AppFile> file2 = GetAppFile("fileId2",
                                                "url2",
                                                1 /*sizeInBytes*/,
                                                {{HashType::Sha256, "sha256"}},
                                                {Architecture::Amd64},
                                                {"myPlatformApplicabilityForPackage1"},
                                                "fileMoniker1");

    std::vector<AppFile> files;
    files.push_back(std::move(*file1));
    files.push_back(std::move(*file2));

    std::vector<AppFile> prereqFiles;
    prereqFiles.push_back(std::move(*GetAppFile("prereqFileId",
                                                "url",
                                                1 /*sizeInBytes*/,
                                                {{HashType::Sha1, "sha1"}},
                                                {Architecture::Amd64},
                                                {"myPlatformApplicabilityForPackage"},
                                                "fileMoniker")));

    std::vector<AppPrerequisiteContent> prerequisites;
    prerequisites.push_back(
        std::move(*GetPrerequisiteContent(contentNameSpace, "prereqName", "prereqVersion", prereqFiles)));

    std::unique_ptr<AppContent> appContent =
        GetAppContent(contentNameSpace, contentName, contentVersion, "updateId", prerequisites, files);
    REQUIRE(appContent != nullptr);
}

TEST("Testing AppContent equality operators")
{
    const std::string contentNameSpace{"myNameSpace"};
    const std::string contentName{"myName"};
    const std::string contentVersion{"myVersion"};
    const std::string updateId{"myUpdateId"};

    std::unique_ptr<AppFile> file = GetAppFile("fileId",
                                               "url",
                                               1 /*sizeInBytes*/,
                                               {{HashType::Sha1, "sha1"}},
                                               {Architecture::Amd64},
                                               {"myPlatformApplicabilityForPackage"},
                                               "fileMoniker");

    std::vector<AppFile> files;
    files.push_back(std::move(*file));

    std::vector<AppFile> prereqFiles;
    prereqFiles.push_back(std::move(*GetAppFile("prereqFileId",
                                                "url",
                                                1 /*sizeInBytes*/,
                                                {{HashType::Sha1, "sha1"}},
                                                {Architecture::Amd64},
                                                {"myPlatformApplicabilityForPackage"},
                                                "fileMoniker")));

    std::vector<AppPrerequisiteContent> prerequisites;
    prerequisites.push_back(
        std::move(*GetPrerequisiteContent(contentNameSpace, "prereqName", "prereqVersion", prereqFiles)));

    const std::unique_ptr<AppContent> content =
        GetAppContent(contentNameSpace, contentName, contentVersion, updateId, prerequisites, files);

    SECTION("Equal")
    {
        auto CompareAppContentEqual = [&content](const std::unique_ptr<AppContent>& sameContent) {
            REQUIRE((*content == *sameContent));
            REQUIRE_FALSE((*content != *sameContent));
        };

        CompareAppContentEqual(
            GetAppContent(contentNameSpace, contentName, contentVersion, updateId, prerequisites, files));
    }

    SECTION("Not equal")
    {
        auto CompareAppContentNotEqual = [&content](const std::unique_ptr<AppContent>& otherContent) {
            REQUIRE((*content != *otherContent));
            REQUIRE_FALSE((*content == *otherContent));
        };

        CompareAppContentNotEqual(GetAppContent("", contentName, contentVersion, updateId, prerequisites, files));
        CompareAppContentNotEqual(GetAppContent(contentNameSpace, "", contentVersion, updateId, prerequisites, files));
        CompareAppContentNotEqual(GetAppContent(contentNameSpace, contentName, "", updateId, prerequisites, files));
        CompareAppContentNotEqual(
            GetAppContent(contentNameSpace, contentName, contentVersion, "", prerequisites, files));
        CompareAppContentNotEqual(GetAppContent(contentNameSpace, contentName, contentVersion, updateId, {}, files));
        CompareAppContentNotEqual(
            GetAppContent(contentNameSpace, contentName, contentVersion, updateId, prerequisites, {}));
    }
}
