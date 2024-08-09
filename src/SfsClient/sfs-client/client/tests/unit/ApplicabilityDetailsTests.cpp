// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ApplicabilityDetails.h"
#include "ContentUtil.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[ApplicabilityDetailsTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details::contentutil;

namespace
{
std::unique_ptr<ApplicabilityDetails> GetDetails(const std::vector<Architecture>& architectures,
                                                 const std::vector<std::string>& platformApplicabilityForPackage)
{
    std::unique_ptr<ApplicabilityDetails> details;
    REQUIRE(ApplicabilityDetails::Make(architectures, platformApplicabilityForPackage, details) == Result::Success);
    REQUIRE(details != nullptr);
    return details;
};
} // namespace

TEST("Testing ApplicabilityDetails::Make()")
{
    const std::vector<Architecture> architectures{Architecture::x86, Architecture::Amd64};
    const std::vector<std::string> platformApplicabilityForPackage{"Windows.Desktop", "Windows.Server"};
    const std::string fileMoniker{"myApp"};

    const std::unique_ptr<ApplicabilityDetails> details = GetDetails(architectures, platformApplicabilityForPackage);

    CHECK(architectures == details->GetArchitectures());
    CHECK(platformApplicabilityForPackage == details->GetPlatformApplicabilityForPackage());

    SECTION("Testing ApplicabilityDetails equality operators")
    {
        SECTION("Equal")
        {
            auto CompareDetailsEqual = [&details](const std::unique_ptr<ApplicabilityDetails>& sameDetails) {
                REQUIRE((*details == *sameDetails));
                REQUIRE_FALSE((*details != *sameDetails));
            };

            CompareDetailsEqual(GetDetails(architectures, platformApplicabilityForPackage));
        }

        SECTION("Not equal")
        {
            auto CompareDetailsNotEqual = [&details](const std::unique_ptr<ApplicabilityDetails>& otherDetails) {
                REQUIRE((*details != *otherDetails));
                REQUIRE_FALSE((*details == *otherDetails));
            };

            CompareDetailsNotEqual(GetDetails({}, platformApplicabilityForPackage));
            CompareDetailsNotEqual(GetDetails(architectures, {}));
            CompareDetailsNotEqual(GetDetails({}, {}));
        }
    }
}
