// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/NameNormalization.h>

using namespace std::string_view_literals;
using namespace AppInstaller::Utility;


// If this test is failing, either changes to winget code or the ICU binaries have caused it.
// This will impact the functionality of the PreIndexedPackageSource, as it is the primary
// mechanism used to cross reference packages installed outside of winget with those in the
// source.
TEST_CASE("NameNorm_Database_Initial", "[name_norm]")
{
    std::ifstream namesStream(TestCommon::TestDataFile("InputNames.txt"));
    std::ifstream publishersStream(TestCommon::TestDataFile("InputPublishers.txt"));
    std::ifstream resultsStream(TestCommon::TestDataFile("NormalizationInitialIds.txt"));

    // Far larger than any one value; hopefully
    char name[4096]{};
    char publisher[4096]{};
    char id[4096]{};

    NameNormalizer normer(NormalizationVersion::Initial);

    for (;;)
    {
        namesStream.getline(name, ARRAYSIZE(name));
        publishersStream.getline(publisher, ARRAYSIZE(publisher));
        resultsStream.getline(id, ARRAYSIZE(id));

        if (namesStream || publishersStream || resultsStream)
        {
            REQUIRE(namesStream);
            REQUIRE(publishersStream);
            REQUIRE(resultsStream);

            INFO("Name[" << name << "], Publisher[" << publisher << "]");

            auto normalized = normer.Normalize(name, publisher);

            std::string expected = normalized.Publisher();
            expected += '.';
            expected += normalized.Name();

            REQUIRE(expected == id);
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
