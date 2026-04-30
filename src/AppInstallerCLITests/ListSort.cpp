// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Workflows/ListSortHelper.h"

using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Settings;

namespace
{
    SortablePackageEntry MakeEntry(std::string name, std::string id, std::string version, std::string available = {}, std::string source = {})
    {
        return SortablePackageEntry{ 0, name, id, version, available, source };
    }

    std::vector<std::string> GetNames(const std::vector<SortablePackageEntry>& entries)
    {
        std::vector<std::string> names;
        for (const auto& e : entries) { names.push_back(e.FoldedName); }
        return names;
    }

    std::vector<std::string> GetIds(const std::vector<SortablePackageEntry>& entries)
    {
        std::vector<std::string> ids;
        for (const auto& e : entries) { ids.push_back(e.FoldedId); }
        return ids;
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

    SortEntries(entries, { SortField::Name }, SortDirection::Ascending);

    auto names = GetNames(entries);
    REQUIRE(names == std::vector<std::string>{ "alpha", "beta", "charlie" });
}

TEST_CASE("ListSort_SortEntries_ByName_Descending", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Alpha", "a", "1.0"),
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
    };

    SortEntries(entries, { SortField::Name }, SortDirection::Descending);

    auto names = GetNames(entries);
    REQUIRE(names == std::vector<std::string>{ "charlie", "beta", "alpha" });
}

TEST_CASE("ListSort_SortEntries_ByName_CaseInsensitive", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("charlie", "c", "1.0"),
        MakeEntry("ALPHA", "a", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
    };

    SortEntries(entries, { SortField::Name }, SortDirection::Ascending);

    auto ids = GetIds(entries);
    REQUIRE(ids == std::vector<std::string>{ "a", "b", "c" });
}

TEST_CASE("ListSort_SortEntries_ById", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Z App", "com.zeta", "1.0"),
        MakeEntry("A App", "com.alpha", "1.0"),
        MakeEntry("M App", "com.mu", "1.0"),
    };

    SortEntries(entries, { SortField::Id }, SortDirection::Ascending);

    auto ids = GetIds(entries);
    REQUIRE(ids == std::vector<std::string>{ "com.alpha", "com.mu", "com.zeta" });
}

TEST_CASE("ListSort_SortEntries_ByVersion", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("App", "app", "10.0.0"),
        MakeEntry("App", "app", "2.0.0"),
        MakeEntry("App", "app", "1.0.0"),
    };

    SortEntries(entries, { SortField::Version }, SortDirection::Ascending);

    // Version is precomputed, verify order via ParsedInstalledVersion
    REQUIRE(entries[0].ParsedInstalledVersion < entries[1].ParsedInstalledVersion);
    REQUIRE(entries[1].ParsedInstalledVersion < entries[2].ParsedInstalledVersion);
}

TEST_CASE("ListSort_SortEntries_MultiField", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Beta", "b.2", "2.0"),
        MakeEntry("Alpha", "a.1", "1.0"),
        MakeEntry("Beta", "b.1", "1.0"),
        MakeEntry("Alpha", "a.2", "2.0"),
    };

    SortEntries(entries, { SortField::Name, SortField::Id }, SortDirection::Ascending);

    auto ids = GetIds(entries);
    REQUIRE(ids == std::vector<std::string>{ "a.1", "a.2", "b.1", "b.2" });
}

TEST_CASE("ListSort_SortEntries_Available_GroupsByPresence", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("NoUpdate1", "n1", "1.0", ""),
        MakeEntry("HasUpdate", "h1", "1.0", "2.0"),
        MakeEntry("NoUpdate2", "n2", "1.0", ""),
    };

    SortEntries(entries, { SortField::Available }, SortDirection::Ascending);

    auto names = GetNames(entries);
    // Has-update first, then no-update (stable within groups)
    REQUIRE(names[0] == "hasupdate");
    REQUIRE(names[1] == "noupdate1");
    REQUIRE(names[2] == "noupdate2");
}

TEST_CASE("ListSort_SortEntries_Relevance_NoOp", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Alpha", "a", "1.0"),
        MakeEntry("Beta", "b", "1.0"),
    };

    // Relevance means preserve original order
    SortEntries(entries, { SortField::Relevance }, SortDirection::Ascending);

    auto names = GetNames(entries);
    REQUIRE(names == std::vector<std::string>{ "charlie", "alpha", "beta" });
}

TEST_CASE("ListSort_SortEntries_EmptyFields_NoOp", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Charlie", "c", "1.0"),
        MakeEntry("Alpha", "a", "1.0"),
    };

    // Empty sort fields means no sorting
    SortEntries(entries, {}, SortDirection::Ascending);

    auto names = GetNames(entries);
    REQUIRE(names == std::vector<std::string>{ "charlie", "alpha" });
}

TEST_CASE("ListSort_SortEntries_SingleElement", "[listsort]")
{
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Only", "only", "1.0"),
    };

    SortEntries(entries, { SortField::Name }, SortDirection::Ascending);

    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0].FoldedName == "only");
}

TEST_CASE("ListSort_SortEntries_StableSort", "[listsort]")
{
    // Two entries with same Name — stable sort preserves original order
    std::vector<SortablePackageEntry> entries = {
        MakeEntry("Same", "first", "1.0"),
        MakeEntry("Same", "second", "1.0"),
    };

    SortEntries(entries, { SortField::Name }, SortDirection::Ascending);

    REQUIRE(entries[0].FoldedId == "first");
    REQUIRE(entries[1].FoldedId == "second");
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
            return SortablePackageEntry(i, r.name, r.id, r.ver, "", "");
        },
        { SortField::Name }, SortDirection::Ascending);

    REQUIRE(rows[0].name == "Alpha");
    REQUIRE(rows[0].extra == 200);
    REQUIRE(rows[1].name == "Beta");
    REQUIRE(rows[2].name == "Charlie");
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
            return SortablePackageEntry(i, r.name, "", "", "", "");
        },
        { SortField::Name }, SortDirection::Ascending);

    REQUIRE(rows[0].payload == "payload-a");
    REQUIRE(rows[1].payload == "payload-z");
}
