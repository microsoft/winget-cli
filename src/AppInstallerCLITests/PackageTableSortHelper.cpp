// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Workflows/PackageTableSortHelper.h"

using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;

namespace
{
    // Use all-fields mask for tests so every field is precomputed.
    constexpr SortField AllFieldsMask =
        SortField::Name | SortField::Id |
        SortField::Version | SortField::Source |
        SortField::Available | SortField::Relevance;

    SortablePackageEntry MakeEntry(std::string name, std::string id, std::string version, std::string available = {}, std::string source = {},
        PackageMatchFilter matchCriteria = { PackageMatchField::Id, MatchType::Wildcard })
    {
        return SortablePackageEntry{ 0, name, id, version, available, source, AllFieldsMask, GetMatchCriteriaSortKey(matchCriteria) };
    }

    // Validates that all precomputed fields in actual match expected, entry by entry.
    void ValidateSortResult(
        const std::vector<SortablePackageEntry>& actual,
        const std::vector<SortablePackageEntry>& expected)
    {
        REQUIRE(actual.size() == expected.size());
        for (size_t i = 0; i < actual.size(); ++i)
        {
            INFO("Entry index: " << i);
            REQUIRE(actual[i].Name == expected[i].Name);
            REQUIRE(actual[i].FoldedId == expected[i].FoldedId);
            REQUIRE(actual[i].FoldedSource == expected[i].FoldedSource);
            REQUIRE(actual[i].ParsedInstalledVersion == expected[i].ParsedInstalledVersion);
            REQUIRE(actual[i].ParsedAvailableVersion == expected[i].ParsedAvailableVersion);
            REQUIRE(actual[i].RelevanceKey == expected[i].RelevanceKey);
        }
    }

    SortParameters MakeSortParams(std::vector<SortField> fields, SortDirection direction = SortDirection::Ascending)
    {
        SortParameters params;
        params.Fields = std::move(fields);
        params.Direction = direction;
        params.ShouldSort = !params.Fields.empty();
        return params;
    }

}

TEST_CASE("ListSort_CompareByField_Name", "[listsort]")
{
    auto a = MakeEntry("Alpha", "a.id", "1.0");
    auto b = MakeEntry("Beta", "b.id", "1.0");

    SECTION("Less than")
    {
        REQUIRE(CompareByField(a, b, SortField::Name) < 0);
    }
    SECTION("Greater than")
    {
        REQUIRE(CompareByField(b, a, SortField::Name) > 0);
    }
    SECTION("Equal")
    {
        REQUIRE(CompareByField(a, a, SortField::Name) == 0);
    }
    SECTION("Case-insensitive")
    {
        auto upper = MakeEntry("ALPHA", "a.id", "1.0");
        REQUIRE(CompareByField(a, upper, SortField::Name) == 0);
    }
}

TEST_CASE("ListSort_CompareByField_Id", "[listsort]")
{
    auto a = MakeEntry("Name", "com.alpha", "1.0");
    auto b = MakeEntry("Name", "com.beta", "1.0");

    REQUIRE(CompareByField(a, b, SortField::Id) < 0);
    REQUIRE(CompareByField(b, a, SortField::Id) > 0);

    SECTION("Case-insensitive")
    {
        auto upper = MakeEntry("Name", "COM.ALPHA", "1.0");
        REQUIRE(CompareByField(a, upper, SortField::Id) == 0);
    }
    SECTION("Numbers remain lexical")
    {
        auto two = MakeEntry("App 2", "com.app.2", "1.0");
        auto ten = MakeEntry("App 10", "com.app.10", "1.0");
        REQUIRE(CompareByField(two, ten, SortField::Id) > 0);
        REQUIRE(CompareByField(two, ten, SortField::Name) < 0);
    }
}

TEST_CASE("ListSort_CompareByField_Name_Natural", "[listsort]")
{
    auto compare = [](std::string a, std::string b)
    {
        return CompareByField(MakeEntry(a, "a", "1.0"), MakeEntry(b, "b", "1.0"), SortField::Name);
    };

    REQUIRE(compare("Microsoft .NET Runtime 3.1", "Microsoft .NET Runtime 9.0") < 0);
    REQUIRE(compare("Microsoft .NET Runtime 9.0", "Microsoft .NET Runtime 10.0") < 0);
    REQUIRE(compare("App 9.9", "App 9.10") < 0);
    REQUIRE(compare("App 10.0", "App 10.0 Preview") < 0);
    REQUIRE(compare("App 09", "App 9") < 0);
    REQUIRE(compare("App 00", "App 0") < 0);
    REQUIRE(compare("App 09b", "App 9a") < 0);
    REQUIRE(compare("co-op", "coop") < 0);
    REQUIRE(compare("", "App") < 0);
    REQUIRE(compare("", "") == 0);
    REQUIRE(compare("Stra\xC3\x9F" "e 10", "STRASSE 10") == 0);
    REQUIRE(compare("\xC3\x84pp 9", "\xC3\xA4PP 9") == 0);
}

TEST_CASE("ListSort_SortEntries_RelevanceAndNaturalName", "[listsort]")
{
    auto direction = GENERATE(SortDirection::Ascending, SortDirection::Descending);
    auto make = [](std::string name, std::string id, MatchType type, PackageMatchField field)
    {
        return MakeEntry(name, id, "1.0", "", "", PackageMatchFilter{ field, type });
    };
    auto weakId = make("A weak match", "weak", MatchType::Substring, PackageMatchField::Id);
    auto exactName = make("A name match", "name", MatchType::Exact, PackageMatchField::Name);
    auto ten = make("Runtime 10.0", "ten", MatchType::Exact, PackageMatchField::Id);
    auto nine = make("Runtime 9.0", "nine", MatchType::Exact, PackageMatchField::Id);
    auto tie = make("RUNTIME 9.0", "tie", MatchType::Exact, PackageMatchField::Id);
    std::vector<SortablePackageEntry> entries{ weakId, exactName, ten, nine, tie };
    std::vector<SortablePackageEntry> expected;

    SECTION("Natural names within relevance buckets")
    {
        SortEntries(entries, MakeSortParams({ SortField::Relevance, SortField::Name }, direction));
        expected = direction == SortDirection::Ascending ?
            std::vector{ nine, tie, ten, exactName, weakId } : std::vector{ weakId, exactName, ten, nine, tie };
    }
    SECTION("Relevance alone preserves ties")
    {
        SortEntries(entries, MakeSortParams({ SortField::Relevance }, direction));
        expected = direction == SortDirection::Ascending ?
            std::vector{ ten, nine, tie, exactName, weakId } : std::vector{ weakId, exactName, ten, nine, tie };
    }
    SECTION("Explicit name sort replaces relevance")
    {
        SortEntries(entries, MakeSortParams({ SortField::Name }, direction));
        expected = direction == SortDirection::Ascending ?
            std::vector{ exactName, weakId, nine, tie, ten } : std::vector{ ten, nine, tie, weakId, exactName };
    }
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_CompareByField_Version", "[listsort]")
{
    auto v1 = MakeEntry("App", "app", "1.0.0");
    auto v2 = MakeEntry("App", "app", "2.0.0");
    auto v10 = MakeEntry("App", "app", "10.0.0");

    SECTION("Semantic ordering")
    {
        REQUIRE(CompareByField(v1, v2, SortField::Version) < 0);
        REQUIRE(CompareByField(v2, v1, SortField::Version) > 0);
    }
    SECTION("Numeric not lexicographic - 10.0 > 2.0")
    {
        REQUIRE(CompareByField(v10, v2, SortField::Version) > 0);
    }
    SECTION("Equal versions")
    {
        REQUIRE(CompareByField(v1, v1, SortField::Version) == 0);
    }
}

TEST_CASE("ListSort_CompareByField_Source", "[listsort]")
{
    auto a = MakeEntry("App", "app", "1.0", "", "msstore");
    auto b = MakeEntry("App", "app", "1.0", "", "winget");

    REQUIRE(CompareByField(a, b, SortField::Source) < 0);
    REQUIRE(CompareByField(b, a, SortField::Source) > 0);
}

TEST_CASE("ListSort_CompareByField_Available", "[listsort]")
{
    auto withUpdate = MakeEntry("App", "app", "1.0", "2.0");
    auto noUpdate = MakeEntry("App", "app", "1.0", "");
    auto higherUpdate = MakeEntry("App", "app", "1.0", "3.0");

    SECTION("Has-update before no-update in ascending")
    {
        REQUIRE(CompareByField(withUpdate, noUpdate, SortField::Available) < 0);
        REQUIRE(CompareByField(noUpdate, withUpdate, SortField::Available) > 0);
    }
    SECTION("Both have updates - compare versions")
    {
        REQUIRE(CompareByField(withUpdate, higherUpdate, SortField::Available) < 0);
        REQUIRE(CompareByField(higherUpdate, withUpdate, SortField::Available) > 0);
    }
    SECTION("Both empty - equal")
    {
        REQUIRE(CompareByField(noUpdate, noUpdate, SortField::Available) == 0);
    }
}

TEST_CASE("ListSort_SortEntries_ByName_Ascending", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Alpha", "a", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Name }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("Alpha", "a", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
        MakeEntry("Charlie", "c", "1.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_ByName_Descending", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Alpha", "a", "1.0"),
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Name }, SortDirection::Descending));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
        MakeEntry("Alpha", "a", "1.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_ByName_CaseInsensitive", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("charlie", "c", "1.0"),
        MakeEntry("ALPHA", "a", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Name }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("ALPHA", "a", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
        MakeEntry("charlie", "c", "1.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_ById", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Z App", "com.zeta", "1.0"),
        MakeEntry("A App", "com.alpha", "1.0"),
        MakeEntry("M App", "com.mu", "1.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Id }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("A App", "com.alpha", "1.0"),
        MakeEntry("M App", "com.mu", "1.0"),
        MakeEntry("Z App", "com.zeta", "1.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_ByVersion", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("App C", "c", "10.0.0"),
        MakeEntry("App A", "a", "2.0.0"),
        MakeEntry("App B", "b", "1.0.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Version }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("App B", "b", "1.0.0"),
        MakeEntry("App A", "a", "2.0.0"),
        MakeEntry("App C", "c", "10.0.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_MultiField", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Beta", "b.2", "2.0"),
        MakeEntry("Alpha", "a.1", "1.0"),
        MakeEntry("Beta", "b.1", "1.0"),
        MakeEntry("Alpha", "a.2", "2.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Name, SortField::Id }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("Alpha", "a.1", "1.0"),
        MakeEntry("Alpha", "a.2", "2.0"),
        MakeEntry("Beta", "b.1", "1.0"),
        MakeEntry("Beta", "b.2", "2.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_Available_GroupsByPresence", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("NoUpdate1", "n1", "1.0", ""),
        MakeEntry("HasUpdate", "h1", "1.0", "2.0"),
        MakeEntry("NoUpdate2", "n2", "1.0", ""),
    };

    SortEntries(entries, MakeSortParams({ SortField::Available }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("HasUpdate", "h1", "1.0", "2.0"),
        MakeEntry("NoUpdate1", "n1", "1.0", ""),
        MakeEntry("NoUpdate2", "n2", "1.0", ""),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_Relevance_StableTies", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Alpha", "a", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Relevance }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Alpha", "a", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_EmptyFields_NoOp", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Alpha", "a", "1.0"),
    };

    // Empty sort fields means no sorting
    SortEntries(entries, SortParameters{});

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Alpha", "a", "1.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_SingleElement", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Only", "only", "1.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Name }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("Only", "only", "1.0"),
    };
    ValidateSortResult(entries, expected);
}

TEST_CASE("ListSort_SortEntries_StableSort", "[listsort]")
{
    // Two entries with same Name — stable sort preserves original order
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Same", "first", "1.0"),
        MakeEntry("Same", "second", "1.0"),
    };

    SortEntries(entries, MakeSortParams({ SortField::Name }));

    std::vector<SortablePackageEntry> expected = {
        MakeEntry("Same", "first", "1.0"),
        MakeEntry("Same", "second", "1.0"),
    };
    ValidateSortResult(entries, expected);
}

// Tests for SortBy template — validates the production sort pipeline
// that converts arbitrary types to SortablePackageEntry and reorders in place.
TEST_CASE("ListSort_SortBy_ReordersSourceItems", "[listsort]")
{
    struct Row { std::string name; std::string id; std::string ver; int extra; };

    std::vector<Row> rows = {
        { "Charlie", "c", "1.0", 100 },
        { "Alpha",   "a", "1.0", 200 },
        { "Beta",    "b", "1.0", 300 },
    };

    SortBy(rows,
        [](const Row& r, size_t i) {
            return SortablePackageEntry(i, r.name, r.id, r.ver, "", "", AllFieldsMask,
                GetMatchCriteriaSortKey(PackageMatchFilter{ PackageMatchField::Id, MatchType::Wildcard }));
        },
        MakeSortParams({ SortField::Name }));

    // Verify all fields of each row after sort
    REQUIRE(rows.size() == 3);
    REQUIRE(rows[0].name == "Alpha");
    REQUIRE(rows[0].id == "a");
    REQUIRE(rows[0].ver == "1.0");
    REQUIRE(rows[0].extra == 200);
    REQUIRE(rows[1].name == "Beta");
    REQUIRE(rows[1].id == "b");
    REQUIRE(rows[1].ver == "1.0");
    REQUIRE(rows[1].extra == 300);
    REQUIRE(rows[2].name == "Charlie");
    REQUIRE(rows[2].id == "c");
    REQUIRE(rows[2].ver == "1.0");
    REQUIRE(rows[2].extra == 100);
}

TEST_CASE("ListSort_SortBy_PreservesExtraFields", "[listsort]")
{
    struct Row { std::string name; std::string payload; };

    std::vector<Row> rows = {
        { "Zeta", "payload-z" },
        { "Alpha", "payload-a" },
    };

    SortBy(rows,
        [](const Row& r, size_t i) {
            return SortablePackageEntry(i, r.name, "", "", "", "", AllFieldsMask,
                GetMatchCriteriaSortKey(PackageMatchFilter{ PackageMatchField::Id, MatchType::Wildcard }));
        },
        MakeSortParams({ SortField::Name }));

    // Verify all fields preserved after sort
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0].name == "Alpha");
    REQUIRE(rows[0].payload == "payload-a");
    REQUIRE(rows[1].name == "Zeta");
    REQUIRE(rows[1].payload == "payload-z");
}
