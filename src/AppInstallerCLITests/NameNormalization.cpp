// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/NameNormalization.h>

using namespace std::string_view_literals;
using namespace AppInstaller::Utility;


// This skipped test case can be used to update the test file.
// It writes back to the output content location, so you must manually
// copy the file(s) back to the git managed location to update.
TEST_CASE("NameNorm_Update_Database_Initial", "[.]")
{
    std::ifstream namesStream(TestCommon::TestDataFile("InputNames.txt"));
    REQUIRE(namesStream);
    std::ifstream publishersStream(TestCommon::TestDataFile("InputPublishers.txt"));
    REQUIRE(publishersStream);
    std::ofstream resultsStream(TestCommon::TestDataFile("NormalizationInitialIdsUpdate.txt"), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
    REQUIRE(resultsStream);

    // Far larger than any one value; hopefully
    char name[4096]{};
    char publisher[4096]{};

    NameNormalizer normer(NormalizationVersion::Initial);

    for (;;)
    {
        namesStream.getline(name, ARRAYSIZE(name));
        publishersStream.getline(publisher, ARRAYSIZE(publisher));

        if (namesStream || publishersStream)
        {
            REQUIRE(namesStream);
            REQUIRE(publishersStream);

            INFO("Name[" << name << "], Publisher[" << publisher << "]");

            auto normalized = normer.Normalize(name, publisher);

            std::string normalizedId = normalized.Publisher();
            normalizedId += '.';
            normalizedId += normalized.Name();

            resultsStream << normalizedId << std::endl;
            REQUIRE(resultsStream);
        }
        else
        {
            break;
        }
    }
}

// If this test is failing, either changes to winget code or the ICU binaries have caused it.
// This will impact the functionality of the PreIndexedPackageSource, as it is the primary
// mechanism used to cross reference packages installed outside of winget with those in the
// source.
TEST_CASE("NameNorm_Database_Initial", "[name_norm]")
{
    std::ifstream namesStream(TestCommon::TestDataFile("InputNames.txt"));
    REQUIRE(namesStream);
    std::ifstream publishersStream(TestCommon::TestDataFile("InputPublishers.txt"));
    REQUIRE(publishersStream);
    std::ifstream resultsStream(TestCommon::TestDataFile("NormalizationInitialIds.txt"));
    REQUIRE(resultsStream);

    // Far larger than any one value; hopefully
    char name[4096]{};
    char publisher[4096]{};
    char expectedId[4096]{};

    NameNormalizer normer(NormalizationVersion::Initial);

    for (;;)
    {
        namesStream.getline(name, ARRAYSIZE(name));
        publishersStream.getline(publisher, ARRAYSIZE(publisher));
        resultsStream.getline(expectedId, ARRAYSIZE(expectedId));

        if (namesStream || publishersStream || resultsStream)
        {
            REQUIRE(namesStream);
            REQUIRE(publishersStream);
            REQUIRE(resultsStream);

            INFO("Name[" << name << "], Publisher[" << publisher << "]");

            auto normalized = normer.Normalize(name, publisher);

            std::string normalizedId = normalized.Publisher();
            normalizedId += '.';
            normalizedId += normalized.Name();

            REQUIRE(expectedId == normalizedId);
        }
        else
        {
            break;
        }
    }
}

TEST_CASE("NameNorm_Architecture", "[name_norm]")
{
    NameNormalizer normer(NormalizationVersion::Initial);

    REQUIRE(normer.Normalize("Name", {}).Architecture() == Architecture::Unknown);
    REQUIRE(normer.Normalize("Name x86", {}).Architecture() == Architecture::X86);
    REQUIRE(normer.Normalize("Name x86_64", {}).Architecture() == Architecture::X64);
    REQUIRE(normer.Normalize("Name (64 bit)", {}).Architecture() == Architecture::X64);
    REQUIRE(normer.Normalize("Name 32/64 bit", {}).Architecture() == Architecture::Unknown);
    REQUIRE(normer.Normalize("Fox86", {}).Architecture() == Architecture::Unknown);
}

TEST_CASE("NameNorm_Locale", "[name_norm]")
{
    NameNormalizer normer(NormalizationVersion::Initial);

    REQUIRE(normer.Normalize("Name", {}).Locale() == "");
    REQUIRE(normer.Normalize("Name en-US", {}).Locale() == "en-us");
    REQUIRE(normer.Normalize("Name (es-mx)", {}).Locale() == "es-mx");
    REQUIRE(normer.Normalize("Names-mx", {}).Locale() == "");
}

TEST_CASE("NameNorm_KBNumbers", "[name_norm]")
{
    NameNormalizer normer(NormalizationVersion::Initial);

    REQUIRE(normer.Normalize("Fix for (KB42)", {}).Name() == "FixforKB42");
}
