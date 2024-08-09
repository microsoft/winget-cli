// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentUtil.h"
#include "sfsclient/ContentId.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[ContentIdTests] " __VA_ARGS__)
#define TEST_SCENARIO(...) TEST_CASE("[ContentIdTests] Scenario: " __VA_ARGS__)

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
};
} // namespace

TEST("Testing ContentId::Make()")
{
    const std::string nameSpace{"myNameSpace"};
    const std::string name{"myName"};
    const std::string version{"myVersion"};

    const std::unique_ptr<ContentId> contentId = GetContentId(nameSpace, name, version);

    CHECK(nameSpace == contentId->GetNameSpace());
    CHECK(name == contentId->GetName());
    CHECK(version == contentId->GetVersion());

    SECTION("Testing ContentId equality operators")
    {
        SECTION("Equal")
        {
            auto CompareContentIdEqual = [&contentId](const std::unique_ptr<ContentId>& sameContentId) {
                REQUIRE((*contentId == *sameContentId));
                REQUIRE_FALSE((*contentId != *sameContentId));
            };

            CompareContentIdEqual(GetContentId(nameSpace, name, version));
        }

        SECTION("Not equal")
        {
            auto CompareContentIdNotEqual = [&contentId](const std::unique_ptr<ContentId>& otherContentId) {
                REQUIRE((*contentId != *otherContentId));
                REQUIRE_FALSE((*contentId == *otherContentId));
            };

            CompareContentIdNotEqual(GetContentId("", name, version));
            CompareContentIdNotEqual(GetContentId(nameSpace, "", version));
            CompareContentIdNotEqual(GetContentId(nameSpace, name, ""));
            CompareContentIdNotEqual(GetContentId("", "", ""));
            CompareContentIdNotEqual(GetContentId("MYNAMESPACE", name, version));
            CompareContentIdNotEqual(GetContentId(nameSpace, "MYNAME", version));
            CompareContentIdNotEqual(GetContentId(nameSpace, name, "MYVERSION"));
        }
    }
}
