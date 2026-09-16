// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "SQLiteIndexTestCommon.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
#include <Microsoft/SQLiteIndex.h>
#include <winget/Manifest.h>
#include <winget/SQLiteMetadataTable.h>
#include <winget/SQLiteWrapper.h>

#include <Microsoft/Schema/1_0/IdTable.h>
#include <Microsoft/Schema/2_0/PackageUpdateTrackingTable.h>
#include <Microsoft/Schema/2_1/DeltaTables.h>
#include <Microsoft/Schema/2_1/DeltaViews.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::SQLite;
using namespace AppInstaller::Utility;

using Tracking = Schema::V2_0::PackageUpdateTrackingTable;
namespace Delta = AppInstaller::Repository::Microsoft::Schema::V2_1::Delta;

namespace
{
    // The first version that can produce or consume a delta.
    const SQLiteVersion s_DeltaVersion{ 2, 1 };

    // Runs a query that produces a single integer.
    int64_t GetScalar(const Connection& connection, const std::string& sql)
    {
        Statement statement = Statement::Create(connection, sql);
        REQUIRE(statement.Step());
        return statement.GetColumn<int64_t>(0);
    }

    // Counts the rows in a table.
    int64_t GetRowCount(const Connection& connection, std::string_view tableName)
    {
        return GetScalar(connection, "SELECT COUNT(*) FROM [" + std::string{ tableName } + "]");
    }

    // Reads every value in a single column of a query as a set.
    std::set<std::string> GetStrings(const Connection& connection, const std::string& sql)
    {
        std::set<std::string> result;
        Statement statement = Statement::Create(connection, sql);

        while (statement.Step())
        {
            result.insert(statement.GetColumn<std::string>(0));
        }

        return result;
    }

    std::set<std::string> ToStringSet(const std::vector<NormalizedString>& values)
    {
        return std::set<std::string>(values.begin(), values.end());
    }

    std::set<std::string> ToFoldedStringSet(const std::vector<NormalizedString>& values)
    {
        std::set<std::string> result;
        for (const auto& value : values)
        {
            result.insert(FoldCase(value));
        }
        return result;
    }

    // Reads the rowid that a prepared index gave a package identifier.
    std::optional<rowid_t> GetPreparedPackageRowId(const std::filesystem::path& indexPath, std::string_view packageIdentifier)
    {
        Connection connection = Connection::Create(indexPath.u8string(), Connection::OpenDisposition::ReadOnly);
        Statement statement = Statement::Create(connection, "SELECT rowid FROM packages WHERE id = ?");
        statement.Bind(1, std::string{ packageIdentifier });

        if (statement.Step())
        {
            return statement.GetColumn<rowid_t>(0);
        }

        return {};
    }

    // Reads a package's associated 1:N values through the given connection.
    std::set<std::string> GetOneToManyValues(
        const Connection& connection,
        std::string_view tableName,
        std::string_view valueName,
        std::string_view packageId)
    {
        std::string sql =
            "SELECT [v].[" + std::string{ valueName } + "] FROM [" + std::string{ tableName } + "] AS [v] "
            "JOIN [" + std::string{ tableName } + "_map] AS [m] ON [m].[" + std::string{ valueName } + "] = [v].[rowid] "
            "JOIN [packages] AS [p] ON [p].[rowid] = [m].[package] "
            "WHERE [p].[id] = '" + std::string{ packageId } + "'";

        return GetStrings(connection, sql);
    }

    // Reads a package's system reference values, which are stored directly against the package.
    std::set<std::string> GetSystemReferenceValues(
        const Connection& connection,
        std::string_view tableName,
        std::string_view valueName,
        std::string_view packageId)
    {
        std::string sql =
            "SELECT [s].[" + std::string{ valueName } + "] FROM [" + std::string{ tableName } + "] AS [s] "
            "JOIN [packages] AS [p] ON [p].[rowid] = [s].[package] "
            "WHERE [p].[id] = '" + std::string{ packageId } + "'";

        return GetStrings(connection, sql);
    }

    struct DeltaTestContext
    {
        TempFile WorkingFile{ "delta_working"s, ".db"s };
        TempFile BaselineFile{ "delta_baseline"s, ".db"s };
        TempFile DeltaFile{ "delta_output"s, ".db"s };

        DeltaTestContext() = default;

        // Creates the working index and fills it with the data the baseline will hold.
        DeltaTestContext(std::initializer_list<IndexFields> baselineData)
        {
            CreateWorking(baselineData);
            CaptureBaseline();
        }

        void CreateWorking(std::initializer_list<IndexFields> baselineData)
        {
            SQLiteIndex index = SQLiteIndex::CreateNew(WorkingFile, s_DeltaVersion);
            index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");

            for (const auto& fields : baselineData)
            {
                Manifest manifest = CreateManifest(fields);
                index.AddManifest(manifest, fields.Path);
            }
        }

        // Copies the working index out, prepares it, and designates it as a baseline.
        // Everything the working index does afterwards is what the delta will describe.
        void CaptureBaseline(bool markAsBaseline = true)
        {
            std::filesystem::copy_file(WorkingFile.GetPath(), BaselineFile.GetPath(), std::filesystem::copy_options::overwrite_existing);

            SQLiteIndex prepared = SQLiteIndex::Open(BaselineFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
            prepared.PrepareForPackaging();

            if (markAsBaseline)
            {
                prepared.MarkAsBaseline();
            }

            m_baselineCaptured = true;
        }

        // Opens the working index for the changes that the delta will carry.
        SQLiteIndex OpenWorkingForChanges(bool resetBaseTimeIfNeeded = true)
        {
            SQLiteIndex index = SQLiteIndex::Open(WorkingFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

            if (resetBaseTimeIfNeeded && !m_baseTimeReset)
            {
                index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "");
                m_baseTimeReset = true;
            }

            return index;
        }

        void Add(const IndexFields& fields, bool resetBaseTimeIfNeeded = true)
        {
            SQLiteIndex index = OpenWorkingForChanges(resetBaseTimeIfNeeded);
            Manifest manifest = CreateManifest(fields);
            index.AddManifest(manifest, fields.Path);
        }

        void Update(const IndexFields& fields)
        {
            SQLiteIndex index = OpenWorkingForChanges();
            Manifest manifest = CreateManifest(fields);
            REQUIRE(index.UpdateManifest(manifest, fields.Path));
        }

        void Remove(const IndexFields& fields)
        {
            SQLiteIndex index = OpenWorkingForChanges();
            Manifest manifest = CreateManifest(fields);
            index.RemoveManifest(manifest, fields.Path);
        }

        // Prepares the working index, producing the delta as a side effect.
        void GenerateDelta()
        {
            REQUIRE(m_baselineCaptured);

            SQLiteIndex index = SQLiteIndex::Open(WorkingFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
            index.SetProperty(SQLiteIndex::Property::DeltaBaselineIndexPath, BaselineFile.GetPath().u8string());
            index.SetProperty(SQLiteIndex::Property::DeltaOutputPath, DeltaFile.GetPath().u8string());
            index.PrepareForPackaging();

            m_deltaGenerated = true;
        }

        // Opens the delta on its own, to inspect what generation actually wrote.
        Connection OpenDeltaConnection()
        {
            REQUIRE(std::filesystem::exists(DeltaFile.GetPath()));
            return Connection::Create(DeltaFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);
        }

        // Opens the delta with its baseline attached and the merged views in place, without going
        // through SQLiteIndex, so that a test can read the merged tables directly.
        Connection OpenMergedConnection()
        {
            Connection connection = OpenDeltaConnection();
            Delta::SetupReadMode(connection, DatabaseSpecifier{ BaselineFile.GetPath().u8string(), DatabaseDisposition::Read });
            return connection;
        }

        SQLiteIndex OpenCombined(SQLiteStorageBase::OpenDisposition disposition = SQLiteStorageBase::OpenDisposition::Read)
        {
            REQUIRE(m_deltaGenerated);
            return SQLiteIndex::OpenWithBaseline(DeltaFile.GetPath().u8string(), BaselineFile.GetPath().u8string(), disposition);
        }

        // The working index, once prepared, is an ordinary full index built from the same data that
        // the delta describes. That makes it the reference for equivalence.
        SQLiteIndex OpenFullIndex()
        {
            REQUIRE(m_deltaGenerated);
            return SQLiteIndex::Open(WorkingFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
        }

        std::string DeltaTable(std::string_view baseTableName) const
        {
            return Delta::GetTableName(baseTableName);
        }

        std::string DeltaMapTable(std::string_view baseTableName) const
        {
            return Delta::GetMapTableName(baseTableName);
        }

    private:
        bool m_baselineCaptured = false;
        bool m_baseTimeReset = false;
        bool m_deltaGenerated = false;
    };

    IndexFields MakePackage(
        std::string id,
        std::string name,
        std::vector<NormalizedString> tags = { "t1", "t2" },
        std::vector<NormalizedString> commands = { "c1" },
        std::vector<NormalizedString> packageFamilyNames = {},
        std::vector<NormalizedString> productCodes = {},
        std::string version = "1.0"s)
    {
        std::string path = id + "/" + version;

        return IndexFields{
            id,
            std::move(name),
            "Publisher"s,
            "moniker"s,
            std::move(version),
            ""s,
            std::move(tags),
            std::move(commands),
            std::move(path),
            std::move(packageFamilyNames),
            std::move(productCodes) };
    }

    // Collects the identifiers that a search returns.
    std::set<std::string> GetSearchedIds(const SQLiteIndex& index, const SearchRequest& request = {})
    {
        std::set<std::string> result;

        for (const auto& match : index.Search(request).Matches)
        {
            auto id = index.GetPropertyByPrimaryId(match.first, PackageVersionProperty::Id);
            REQUIRE(id.has_value());
            result.insert(id.value());
        }

        return result;
    }
}

// ---------------------------------------------------------------------------------------------
// Group B - package rowid identity
//
// The merged package's view suppresses a baseline row when the delta names the same rowid, and
// every association refers to a package by that rowid. These cases cover the ways that identity
// can be broken.
// ---------------------------------------------------------------------------------------------

// B1. A package removed and re-added lands on a new rowid, so the delta has to both introduce it
// at the new rowid and vacate the old one. Recording only the add leaves the baseline row in
// place, and the package appears twice.
TEST_CASE("SQLiteIndex_Delta_PackageRemovedThenReAdded", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p3 = MakePackage("Publisher3.Id", "Package 3");

    DeltaTestContext context{ { p1, p2, p3 } };

    rowid_t originalRowId = GetPreparedPackageRowId(context.BaselineFile.GetPath(), p2.Id).value();

    // Removing the middle package frees its rowid, but the next insert takes one above the highest
    // in use, so the re-add cannot land back on it.
    context.Remove(p2);
    context.Add(p2);

    context.GenerateDelta();

    rowid_t newRowId = GetPreparedPackageRowId(context.WorkingFile.GetPath(), p2.Id).value();
    REQUIRE(newRowId != originalRowId);

    {
        Connection delta = context.OpenDeltaConnection();

        // The old rowid must be vacated, or nothing suppresses the baseline row.
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [rowid] = " + std::to_string(originalRowId) + " AND [is_removed] = 1") == 1);
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [rowid] = " + std::to_string(newRowId) + " AND [is_removed] = 0") == 1);
    }

    SQLiteIndex combined = context.OpenCombined();

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, p2.Id));

    // Exactly one, and from the delta rather than the stale baseline row.
    auto results = combined.Search(request);
    REQUIRE(results.Matches.size() == 1);

    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id, p2.Id, p3.Id });
}

// B2. Removing the package that holds the highest rowid frees it for the next package added, so
// a removal and a change can name the same rowid. Writing both violates the delta's primary key.
TEST_CASE("SQLiteIndex_Delta_RemovedPackageRowIdReused", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p3 = MakePackage("Publisher3.Id", "Package 3");

    DeltaTestContext context{ { p1, p2, p3 } };

    rowid_t reusedRowId = GetPreparedPackageRowId(context.BaselineFile.GetPath(), p3.Id).value();

    auto p4 = MakePackage("Publisher4.Id", "Package 4");

    context.Remove(p3);
    context.Add(p4);

    REQUIRE_NOTHROW(context.GenerateDelta());

    // The new package took the rowid that the removed one gave up.
    REQUIRE(GetPreparedPackageRowId(context.WorkingFile.GetPath(), p4.Id).value() == reusedRowId);

    {
        Connection delta = context.OpenDeltaConnection();

        // Only the change is recorded. The removal would be redundant, since a delta row at that
        // rowid already displaces the baseline row, and it cannot be written in any case.
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [rowid] = " + std::to_string(reusedRowId)) == 1);
        REQUIRE(GetStrings(delta, "SELECT [id] FROM [delta_packages] WHERE [rowid] = " + std::to_string(reusedRowId)) ==
            std::set<std::string>{ p4.Id });
    }

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id, p2.Id, p4.Id });
}

// B2 continued. The displaced package's associations must go with it. They are compared against the
// baseline at the shared rowid, so the values belonging to the old occupant are removed.
TEST_CASE("SQLiteIndex_Delta_ReusedRowIdReplacesAssociations", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "keep" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2", { "old1", "old2" }, { "old_cmd" }, {}, { "OLD_PC" });

    DeltaTestContext context{ { p1, p2 } };

    auto p3 = MakePackage("Publisher3.Id", "Package 3", { "new1" }, { "new_cmd" }, {}, { "NEW_PC" });

    context.Remove(p2);
    context.Add(p3);
    context.GenerateDelta();

    Connection merged = context.OpenMergedConnection();

    // Nothing of the old occupant survives at the shared rowid.
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p3.Id) == ToStringSet(p3.Tags));
    REQUIRE(GetOneToManyValues(merged, "commands2", "command", p3.Id) == ToStringSet(p3.Commands));
    REQUIRE(GetSystemReferenceValues(merged, "productcodes2", "productcode", p3.Id) == ToFoldedStringSet(p3.ProductCodes));

    // The untouched package is unaffected.
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p1.Id) == ToStringSet(p1.Tags));
}

// B6. Remove, re-add, and remove again leaves a tombstone for each rowid the package vacated. Both
// are reported, but only one of them names a rowid the baseline holds, so only one removal is
// written — a delta that recorded the transient rowid too would claim to suppress a row that has
// never existed.
TEST_CASE("SQLiteIndex_Delta_RemoveAddRemove", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p3 = MakePackage("Publisher3.Id", "Package 3");

    DeltaTestContext context{ { p1, p2, p3 } };

    rowid_t originalRowId = GetPreparedPackageRowId(context.BaselineFile.GetPath(), p2.Id).value();

    context.Remove(p2);
    context.Add(p2);
    context.Remove(p2);

    {
        Connection working = Connection::Create(context.WorkingFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);

        // One tombstone per vacated rowid, and generation is told about both of them.
        REQUIRE(GetScalar(working, "SELECT COUNT(*) FROM [update_tracking] WHERE [package] = '" + p2.Id + "' AND [is_removed] = 1") == 2);

        auto removals = Tracking::GetRemovalsSince(working, 0, Tracking::RemovalBehavior::Record);
        REQUIRE(removals.size() == 2);
        REQUIRE(removals.count(originalRowId) == 1);
    }

    REQUIRE_NOTHROW(context.GenerateDelta());

    {
        Connection delta = context.OpenDeltaConnection();
        // Only the rowid the baseline actually holds is written; the one the re-add briefly
        // occupied is above the baseline's range, so there is nothing there to suppress.
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [id] = '" + p2.Id + "'") == 1);
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [rowid] = " + std::to_string(originalRowId) + " AND [is_removed] = 1") == 1);
    }

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id, p3.Id });
}

// B14. The same sequence, but the re-add changes the casing of the identifier. The tracking table
// freezes the casing of each row at insert, so the two tombstones carry byte different names. With
// identity settled on the rowid this is uneventful, which is exactly what it is asserting: no part
// of the removal path compares the two spellings, so nothing can get the comparison wrong.
TEST_CASE("SQLiteIndex_Delta_RemoveAddRemove_CasingChanged", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p3 = MakePackage("Publisher3.Id", "Package 3");
    auto p2_NewCase = MakePackage("publisher2.id", "Package 2");

    DeltaTestContext context{ { p1, p2, p3 } };

    rowid_t originalRowId = GetPreparedPackageRowId(context.BaselineFile.GetPath(), p2.Id).value();

    // Publisher3 holds the highest rowid, so the re-add lands on a new one and leaves the first
    // tombstone in place rather than reviving it.
    context.Remove(p2);
    context.Add(p2_NewCase);
    context.Remove(p2_NewCase);

    {
        Connection working = Connection::Create(context.WorkingFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);

        // Two tombstones at two rowids. The differing casing does not participate in identity at
        // all, which is the point: nothing here has to reconcile the two spellings.
        auto removals = Tracking::GetRemovalsSince(working, 0, Tracking::RemovalBehavior::Record);
        REQUIRE(removals.size() == 2);
        REQUIRE(removals.count(originalRowId) == 1);
    }

    REQUIRE_NOTHROW(context.GenerateDelta());

    {
        Connection delta = context.OpenDeltaConnection();

        // Only the baseline rowid was written, and the identifier stored with it is the one the
        // baseline holds rather than either spelling the tracking table recorded.
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [is_removed] = 1") == 1);
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [rowid] = " + std::to_string(originalRowId) + " AND [is_removed] = 1") == 1);
    }

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id, p3.Id });
}

// B7. When the re-add happens to land on the rowid the package just gave up, the tracking row is
// updated in place. Leaving a tombstone next to the live row would contradict it.
TEST_CASE("SQLiteIndex_Delta_ReAddOnSameRowIdUpdatesInPlace", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");

    DeltaTestContext context{ { p1, p2 } };

    rowid_t originalRowId = GetPreparedPackageRowId(context.BaselineFile.GetPath(), p2.Id).value();

    // Publisher2 holds the highest rowid, so removing it frees the value that the re-add takes.
    context.Remove(p2);
    context.Add(p2);

    context.GenerateDelta();

    REQUIRE(GetPreparedPackageRowId(context.WorkingFile.GetPath(), p2.Id).value() == originalRowId);

    Connection delta = context.OpenDeltaConnection();

    // A single live row, and no tombstone left behind to contradict it.
    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [id] = '" + p2.Id + "'") == 1);
    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [id] = '" + p2.Id + "' AND [is_removed] = 0") == 1);

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id, p2.Id });
}

// B8. Two identifiers legitimately share a rowid in the tracking table when one vacates it and the
// other takes it. A unique index on the rowid alone would reject exactly the case that the
// tombstone exists to record; only the live rows are constrained.
TEST_CASE("SQLiteIndex_Delta_TrackingAllowsSharedRowIdAcrossPackages", "[sqliteindex][V2_1][update_tracking]")
{
    TempFile indexFile{ "update_tracking"s, ".db"s };

    ManifestAndPath m1;
    CreateFakeManifestAndPath(m1, "Publisher1", "1.0");
    ManifestAndPath m2;
    CreateFakeManifestAndPath(m2, "Publisher2", "1.0");

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(indexFile, s_DeltaVersion);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");
        index.AddManifest(m1.Manifest, m1.Path);
        index.RemoveManifest(m1.Manifest, m1.Path);

        // Publisher1 gave up the only rowid in use, so Publisher2 takes it.
        REQUIRE_NOTHROW(index.AddManifest(m2.Manifest, m2.Path));
        REQUIRE(index.CheckConsistency(true));
    }

    Connection connection = Connection::Create(indexFile, Connection::OpenDisposition::ReadOnly);

    int64_t sharedRowId = GetScalar(connection, "SELECT [package_rowid] FROM [update_tracking] WHERE [package] = '" + m2.Manifest.Id + "'");
    REQUIRE(GetScalar(connection, "SELECT [package_rowid] FROM [update_tracking] WHERE [package] = '" + m1.Manifest.Id + "'") == sharedRowId);

    REQUIRE(GetScalar(connection, "SELECT COUNT(*) FROM [update_tracking] WHERE [package_rowid] = " + std::to_string(sharedRowId)) == 2);
    REQUIRE(GetScalar(connection, "SELECT COUNT(*) FROM [update_tracking] WHERE [package_rowid] = " + std::to_string(sharedRowId) + " AND [is_removed] = 0") == 1);
}

// B9. Identifiers that differ only by case are the same package. The index decides that with the
// ICU LIKE implementation, which folds beyond ASCII, so a NOCASE index would disagree here.
TEST_CASE("SQLiteIndex_Delta_TrackingIdentityIsIcuCaseInsensitive", "[sqliteindex][V2_1][update_tracking]")
{
    TempFile indexFile{ "update_tracking"s, ".db"s };

    // Cyrillic, where the case mapping is well defined but outside the ASCII range that the
    // built in NOCASE collation folds.
    ManifestAndPath lower;
    CreateFakeManifestAndPath(lower, "\xd0\xbf\xd1\x80\xd0\xb8\xd0\xbc\xd0\xb5\xd1\x80", "1.0");
    ManifestAndPath upper;
    CreateFakeManifestAndPath(upper, "\xd0\x9f\xd0\xa0\xd0\x98\xd0\x9c\xd0\x95\xd0\xa0", "2.0");

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(indexFile, s_DeltaVersion);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");
        index.AddManifest(lower.Manifest, lower.Path);
        index.AddManifest(upper.Manifest, upper.Path);
        REQUIRE(index.CheckConsistency(true));
    }

    Connection connection = Connection::Create(indexFile, Connection::OpenDisposition::ReadOnly);

    // One package, so one live tracking row. Two would mean the two spellings were treated as
    // different packages, and the live row constraint failed to see them as one.
    REQUIRE(GetScalar(connection, "SELECT COUNT(*) FROM [update_tracking] WHERE [is_removed] = 0") == 1);
    REQUIRE(GetScalar(connection, "SELECT COUNT(DISTINCT [package_rowid]) FROM [update_tracking]") == 1);
}

// B10. The rowid the tracking table stores has to be the one the prepared index assigns, since
// that is what generation and the merged views agree on. This also fails if pinning regresses.
TEST_CASE("SQLiteIndex_Delta_TrackingRowIdMatchesPreparedIndex", "[sqliteindex][V2_1][update_tracking]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p3 = MakePackage("Publisher3.Id", "Package 3");

    DeltaTestContext context{ { p1, p2, p3 } };

    std::vector<std::pair<std::string, int64_t>> tracked;

    {
        Connection working = Connection::Create(context.WorkingFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);
        Statement statement = Statement::Create(working, "SELECT [package], [package_rowid] FROM [update_tracking] WHERE [is_removed] = 0");

        while (statement.Step())
        {
            tracked.emplace_back(statement.GetColumn<std::string>(0), statement.GetColumn<int64_t>(1));
        }
    }

    REQUIRE(tracked.size() == 3);

    for (const auto& [packageId, trackedRowId] : tracked)
    {
        INFO(packageId);
        auto preparedRowId = GetPreparedPackageRowId(context.BaselineFile.GetPath(), packageId);
        REQUIRE(preparedRowId.has_value());
        REQUIRE(preparedRowId.value() == trackedRowId);
    }
}

// B12. Package identity is case-insensitive everywhere in the index: the ids table collapses
// LIKE equal identifiers onto one rowid and overwrites the stored string with the most recent
// casing, while the tracking table freezes the casing it first saw. Generation therefore has to
// resolve a package across a casing difference between the two.
//
// On the changed path, byte equality fails to resolve the package and generation throws for the
// entire index.
TEST_CASE("SQLiteIndex_Delta_IdentifierCasingChange_Changed", "[sqliteindex][V2_1][delta]")
{
    auto original = MakePackage("Publisher1.Id", "Package 1");
    auto unchanged = MakePackage("Publisher2.Id", "Package 2");
    auto newCase = MakePackage("publisher1.id", "Package 1 V2", { "t1", "t2" }, { "c1" }, {}, {}, "2.0"s);

    DeltaTestContext context{ { original, unchanged } };

    // Adding a version under a different casing rewrites the ids table entry, and with it the
    // identifier that packaging will put in the packages table. The tracking row keeps the
    // original casing.
    context.Add(newCase);

    REQUIRE_NOTHROW(context.GenerateDelta());

    SQLiteIndex combined = context.OpenCombined();

    // Exactly one row for the package. Resolving to no rowid would have left the baseline row
    // unsuppressed alongside the delta's, showing it twice.
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ newCase.Id, unchanged.Id });
}

// B13. The silent half of the same defect. Here the casing changed before the baseline was taken,
// so the baseline holds the new casing while the tracking table still holds the old. A removal
// that cannot be resolved against the baseline is treated as "never existed there" and skipped,
// leaving the baseline row visible forever.
TEST_CASE("SQLiteIndex_Delta_IdentifierCasingChange_Removed", "[sqliteindex][V2_1][delta]")
{
    auto original = MakePackage("Publisher1.Id", "Package 1");
    auto newCase = MakePackage("publisher1.id", "Package 1 V2", { "t1", "t2" }, { "c1" }, {}, {}, "2.0"s);
    auto unchanged = MakePackage("Publisher2.Id", "Package 2");

    DeltaTestContext context;
    context.CreateWorking({ original, unchanged });
    context.Add(newCase, false);
    context.CaptureBaseline();

    REQUIRE(GetPreparedPackageRowId(context.BaselineFile.GetPath(), newCase.Id).has_value());

    context.Remove(original);
    context.Remove(newCase);

    context.GenerateDelta();

    SQLiteIndex combined = context.OpenCombined();

    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ unchanged.Id });
}

// B11. The migration adds the rowid column to a table whose rows predate it. A backfill that left
// nulls behind would break the first delta generated after an upgrade.
TEST_CASE("SQLiteIndex_Delta_TrackingMigrationBackfillsRowIds", "[sqliteindex][V2_1][update_tracking]")
{
    TempFile indexFile{ "update_tracking"s, ".db"s };

    ManifestAndPath m1;
    CreateFakeManifestAndPath(m1, "Publisher1", "1.0");
    ManifestAndPath m2;
    CreateFakeManifestAndPath(m2, "Publisher2", "1.0");

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(indexFile, SQLiteVersion{ 2, 0 });
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");
        index.AddManifest(m1.Manifest, m1.Path);
        index.AddManifest(m2.Manifest, m2.Path);
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(indexFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(index.MigrateTo(s_DeltaVersion));
        REQUIRE(index.CheckConsistency(true));
    }

    Connection connection = Connection::Create(indexFile, Connection::OpenDisposition::ReadOnly);

    REQUIRE(GetRowCount(connection, "update_tracking") == 2);

    // The column arrives with a default of 0, ensure that they all got mapped to the real value.
    REQUIRE(GetScalar(connection, "SELECT COUNT(*) FROM [update_tracking] WHERE [package_rowid] = 0") == 0);

    // The backfilled value has to be the one the index itself uses, not just any non null.
    for (const auto& packageId : { m1.Manifest.Id, m2.Manifest.Id })
    {
        INFO(packageId);
        int64_t tracked = GetScalar(connection, "SELECT [package_rowid] FROM [update_tracking] WHERE [package] = '" + std::string{ packageId } + "'");
        auto idRowId = Schema::V1_0::IdTable::SelectIdByValue(connection, std::string{ packageId });
        REQUIRE(idRowId.has_value());
        REQUIRE(idRowId.value() == tracked);
    }
}

// B3/B4. Rowids have to survive repeated preparation, not just one round. Two rounds cannot
// distinguish a stable assignment from one that happens to repeat.
TEST_CASE("SQLiteIndex_Delta_RowIdsAreStableAcrossPrepares", "[sqliteindex][V2_0]")
{
    TempFile workingFile{ "rowid_working"s, ".db"s };

    std::vector<ManifestAndPath> manifests(5);
    CreateFakeManifestAndPath(manifests[0], "Publisher1", "1.0");
    CreateFakeManifestAndPath(manifests[1], "Publisher2", "1.0");
    CreateFakeManifestAndPath(manifests[2], "Publisher3", "1.0");
    CreateFakeManifestAndPath(manifests[3], "Publisher4", "1.0");
    CreateFakeManifestAndPath(manifests[4], "Publisher5", "1.0");

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(workingFile, SQLiteVersion{ 2, 0 });
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");
        index.AddManifest(manifests[0].Manifest, manifests[0].Path);
        index.AddManifest(manifests[1].Manifest, manifests[1].Path);
        index.AddManifest(manifests[2].Manifest, manifests[2].Path);
    }

    auto prepareCopy = [&](const TempFile& target)
    {
        std::filesystem::copy_file(workingFile.GetPath(), target.GetPath(), std::filesystem::copy_options::overwrite_existing);
        SQLiteIndex prepared = SQLiteIndex::Open(target.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
        prepared.PrepareForPackaging();
    };

    TempFile first{ "rowid_first"s, ".db"s };
    prepareCopy(first);

    // Round two: drop one package and add another.
    {
        SQLiteIndex index = SQLiteIndex::Open(workingFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.RemoveManifest(manifests[0].Manifest, manifests[0].Path);
        index.AddManifest(manifests[3].Manifest, manifests[3].Path);
    }

    TempFile second{ "rowid_second"s, ".db"s };
    prepareCopy(second);

    // Round three, to catch an assignment that only appears stable over a single step.
    {
        SQLiteIndex index = SQLiteIndex::Open(workingFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.RemoveManifest(manifests[1].Manifest, manifests[1].Path);
        index.AddManifest(manifests[4].Manifest, manifests[4].Path);
    }

    TempFile third{ "rowid_third"s, ".db"s };
    prepareCopy(third);

    // Publisher3 is present throughout and must never move.
    rowid_t p3First = GetPreparedPackageRowId(first.GetPath(), manifests[2].Manifest.Id).value();
    REQUIRE(GetPreparedPackageRowId(second.GetPath(), manifests[2].Manifest.Id).value() == p3First);
    REQUIRE(GetPreparedPackageRowId(third.GetPath(), manifests[2].Manifest.Id).value() == p3First);

    // Publisher4 survives from round two to round three.
    REQUIRE(GetPreparedPackageRowId(third.GetPath(), manifests[3].Manifest.Id).value() == GetPreparedPackageRowId(second.GetPath(), manifests[3].Manifest.Id).value());

    // B5. A package added after the baseline must land above everything the baseline holds, or it
    // would collide with an untouched package when the two are merged.
    rowid_t maxInFirst = 0;
    for (const auto& id : { manifests[0].Manifest.Id, manifests[1].Manifest.Id, manifests[2].Manifest.Id })
    {
        maxInFirst = std::max(maxInFirst, GetPreparedPackageRowId(first.GetPath(), id).value());
    }

    REQUIRE(GetPreparedPackageRowId(second.GetPath(), manifests[3].Manifest.Id).value() > maxInFirst);
}

// ---------------------------------------------------------------------------------------------
// Group C - generation of the packages table
// ---------------------------------------------------------------------------------------------

TEST_CASE("SQLiteIndex_Delta_AddedPackage", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");

    DeltaTestContext context{ { p1 } };

    context.Add(p2);
    context.GenerateDelta();

    Connection delta = context.OpenDeltaConnection();

    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [is_removed] = 0") == 1);
    REQUIRE(GetStrings(delta, "SELECT [id] FROM [delta_packages] WHERE [is_removed] = 0") == std::set<std::string>{ p2.Id });
}

TEST_CASE("SQLiteIndex_Delta_RemovedPackage", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");

    DeltaTestContext context{ { p1, p2 } };

    context.Remove(p2);
    context.GenerateDelta();

    Connection delta = context.OpenDeltaConnection();

    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [is_removed] = 1") == 1);
    REQUIRE(GetStrings(delta, "SELECT [id] FROM [delta_packages] WHERE [is_removed] = 1") == std::set<std::string>{ p2.Id });

    // A removal carries no data beyond identity, so the rest of the row stays null.
    REQUIRE(GetScalar(delta,
        "SELECT COUNT(*) FROM [delta_packages] WHERE [is_removed] = 1 AND [name] IS NULL AND [latest_version] IS NULL AND [hash] IS NULL") == 1);
}

// C4. Only the identifier and name have ever been asserted, so a column dropped from the copy
// would go unnoticed.
TEST_CASE("SQLiteIndex_Delta_ChangedPackageCopiesEveryColumn", "[sqliteindex][V2_1][delta]")
{
    DeltaTestContext context{ { MakePackage("Publisher1.Id", "Package 1") } };
    ManifestAndPath added;
    CreateFakeManifestAndPath(added, "Publisher2", "3.4.5", "1.2"sv, "6.7"sv);

    {
        SQLiteIndex index = context.OpenWorkingForChanges();
        index.AddManifest(added.Manifest, added.Path);
    }

    context.GenerateDelta();

    Connection delta = context.OpenDeltaConnection();
    Connection source = Connection::Create(context.WorkingFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);

    for (std::string_view column : { "id"sv, "name"sv, "moniker"sv, "latest_version"sv, "arp_min_version"sv, "arp_max_version"sv })
    {
        INFO(column);

        auto fromDelta = GetStrings(delta, "SELECT [" + std::string{ column } + "] FROM [delta_packages] WHERE [id] = '" + added.Manifest.Id + "'");
        auto fromSource = GetStrings(source, "SELECT [" + std::string{ column } + "] FROM [packages] WHERE [id] = '" + added.Manifest.Id + "'");

        REQUIRE(fromDelta == fromSource);
        REQUIRE(fromDelta.size() == 1);
        REQUIRE(!fromDelta.begin()->empty());
    }

    // The hash is a blob, so compare it as one rather than through the string accessor.
    Statement deltaHash = Statement::Create(delta, "SELECT [hash] FROM [delta_packages] WHERE [id] = '" + added.Manifest.Id + "'");
    REQUIRE(deltaHash.Step());
    Statement sourceHash = Statement::Create(source, "SELECT [hash] FROM [packages] WHERE [id] = '" + added.Manifest.Id + "'");
    REQUIRE(sourceHash.Step());

    auto hashValue = deltaHash.GetColumn<blob_t>(0);
    REQUIRE(!hashValue.empty());
    REQUIRE(hashValue == sourceHash.GetColumn<blob_t>(0));
}

TEST_CASE("SQLiteIndex_Delta_MultipleChangesAndRemovals", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p3 = MakePackage("Publisher3.Id", "Package 3");
    auto p4 = MakePackage("Publisher4.Id", "Package 4");
    auto p3Updated = MakePackage(p3.Id, "Renamed 3");
    auto p5 = MakePackage("Publisher5.Id", "Package 5");
    auto p6 = MakePackage("Publisher6.Id", "Package 6");

    DeltaTestContext context{ { p1, p2, p3, p4 } };

    context.Remove(p1);
    context.Remove(p2);
    context.Update(p3Updated);
    context.Add(p5);
    context.Add(p6);

    context.GenerateDelta();

    Connection delta = context.OpenDeltaConnection();

    REQUIRE(GetStrings(delta, "SELECT [id] FROM [delta_packages] WHERE [is_removed] = 1") ==
        std::set<std::string>{ p1.Id, p2.Id });
    REQUIRE(GetStrings(delta, "SELECT [id] FROM [delta_packages] WHERE [is_removed] = 0") ==
        std::set<std::string>{ p3Updated.Id, p5.Id, p6.Id });

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p3Updated.Id, p4.Id, p5.Id, p6.Id });
}

// C6. A package that came and went within the window was never in the baseline, so there is
// nothing to suppress and the delta must not claim to remove it.
TEST_CASE("SQLiteIndex_Delta_PackageAddedAndRemovedWithinWindow", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto transient = MakePackage("Transient.Id", "Transient");

    DeltaTestContext context{ { p1 } };

    context.Add(transient);
    context.Remove(transient);

    REQUIRE_NOTHROW(context.GenerateDelta());

    Connection delta = context.OpenDeltaConnection();
    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_packages] WHERE [id] LIKE '" + transient.Id + "'") == 0);

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id });
}

// C8. The identifier lookup used to be a LIKE, which treats these characters as wildcards. A
// package whose identifier contains them would have matched the wrong row.
TEST_CASE("SQLiteIndex_Delta_IdentifierWithLikeWildcards", "[sqliteindex][V2_1][delta]")
{
    auto wild = MakePackage("Publisher_A.Id", "Wildcard Underscore");
    auto decoy = MakePackage("PublisherXA.Id", "Decoy");
    auto percent = MakePackage("Pub%cent.Id", "Wildcard Percent");

    DeltaTestContext context{ { wild, decoy, percent } };

    context.Remove(wild);
    context.GenerateDelta();

    Connection delta = context.OpenDeltaConnection();

    // Only the package actually removed; a LIKE would also have matched the decoy.
    REQUIRE(GetStrings(delta, "SELECT [id] FROM [delta_packages] WHERE [is_removed] = 1") == std::set<std::string>{ wild.Id });

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ decoy.Id, percent.Id });
}

TEST_CASE("SQLiteIndex_Delta_NoChanges_EmptyDelta", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");

    DeltaTestContext context{ { p1 } };

    // Reset the base time without making any change, so nothing is reported.
    context.OpenWorkingForChanges();
    context.GenerateDelta();

    Connection delta = context.OpenDeltaConnection();

    // The delta is a database in its own right, carrying the metadata that any index has. Without
    // it the delta could not be opened, since opening reads the schema version to pick an interface.
    REQUIRE(MetadataTable::GetNamedValue<int64_t>(delta, s_MetadataValueName_MajorVersion) == 2);
    REQUIRE(MetadataTable::GetNamedValue<int64_t>(delta, s_MetadataValueName_MinorVersion) == 1);
    REQUIRE(!MetadataTable::TryGetNamedValue<std::string>(delta, s_MetadataValueName_DatabaseIdentifier).value_or(std::string{}).empty());
    REQUIRE(MetadataTable::TryGetNamedValue<int64_t>(delta, s_MetadataValueName_LastWriteTime).has_value());

    REQUIRE(GetRowCount(delta, "delta_packages") == 0);

    for (const auto& table : Delta::SystemReferenceTables())
    {
        INFO(table.TableName);
        REQUIRE(GetRowCount(delta, Delta::GetTableName(table.TableName)) == 0);
    }

    for (const auto& table : Delta::OneToManyTables())
    {
        INFO(table.TableName);
        REQUIRE(GetRowCount(delta, Delta::GetTableName(table.TableName)) == 0);
        REQUIRE(GetRowCount(delta, Delta::GetMapTableName(table.TableName)) == 0);
    }

    // J4. The delta ships as plain tables, just as a prepared 2.0 index does. Its indexes serve
    // only generation, and the merged views probe nothing but primary keys.
    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [sqlite_master] WHERE [type] = 'index'") == 0);

    // An empty delta still has to merge cleanly.
    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id });
}

// ---------------------------------------------------------------------------------------------
// Group D - generation of the system reference tables
//
// These decide whether ARP correlation still works after an update, and nothing covered them.
// ---------------------------------------------------------------------------------------------

TEST_CASE("SQLiteIndex_Delta_SystemReference_AddAndRemove", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "t1" }, { "c1" }, { "Family1_8wekyb3d8bbwe" }, { "PC-KEEP", "PC-DROP" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2", { "t1" }, { "c1" }, { "Family2_8wekyb3d8bbwe" }, { "PC-OTHER" });
    auto p1Updated = MakePackage(p1.Id, p1.Name, { "t1" }, { "c1" }, { "Family3_8wekyb3d8bbwe" }, { "PC-KEEP", "PC-NEW" });

    DeltaTestContext context{ { p1, p2 } };

    // Trade one product code for another while keeping a third, and swap the family name.
    context.Update(p1Updated);

    context.GenerateDelta();

    {
        Connection delta = context.OpenDeltaConnection();

        // D6. Only the difference is recorded. Writing the whole current set would destroy the size
        // benefit that justifies the feature.
        auto productCodeTable = Delta::GetTableName("productcodes2");
        REQUIRE(GetStrings(delta, "SELECT [productcode] FROM [" + productCodeTable + "] WHERE [is_removed] = 0") ==
            std::set<std::string>{ "pc-new" });
        REQUIRE(GetStrings(delta, "SELECT [productcode] FROM [" + productCodeTable + "] WHERE [is_removed] = 1") ==
            std::set<std::string>{ "pc-drop" });

        // The kept value is not mentioned at all.
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [" + productCodeTable + "] WHERE [productcode] = 'pc-keep'") == 0);
    }

    Connection merged = context.OpenMergedConnection();

    // D3. Suppression is per row: the kept code survives even though the package changed.
    REQUIRE(GetSystemReferenceValues(merged, "productcodes2", "productcode", p1Updated.Id) ==
        std::set<std::string>{ "pc-keep", "pc-new" });
    REQUIRE(GetSystemReferenceValues(merged, "pfns2", "pfn", p1Updated.Id) ==
        std::set<std::string>{ "family3_8wekyb3d8bbwe" });

    // The untouched package keeps everything.
    REQUIRE(GetSystemReferenceValues(merged, "productcodes2", "productcode", p2.Id) ==
        std::set<std::string>{ "pc-other" });
}

// D7. The user visible consequence: correlation by product code has to find the updated package.
TEST_CASE("SQLiteIndex_Delta_SystemReference_CorrelationThroughCombined", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "t1" }, { "c1" }, { "Family1_8wekyb3d8bbwe" }, { "PC-KEEP", "PC-DROP" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p1Updated = MakePackage(p1.Id, p1.Name, { "t1" }, { "c1" }, p1.PackageFamilyNames, { "PC-KEEP", "PC-NEW" });

    DeltaTestContext context{ { p1, p2 } };

    context.Update(p1Updated);
    context.GenerateDelta();

    SQLiteIndex combined = context.OpenCombined();

    for (const auto& productCode : p1Updated.ProductCodes)
    {
        INFO(productCode);

        SearchRequest request;
        request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, std::string{ productCode }));

        REQUIRE(GetSearchedIds(combined, request) == std::set<std::string>{ p1Updated.Id });
    }

    // The dropped code must no longer correlate to anything.
    SearchRequest dropped;
    dropped.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, p1.ProductCodes[1]));
    REQUIRE(combined.Search(dropped).Matches.empty());

    // And the family name still resolves through the merged view.
    SearchRequest family;
    family.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::PackageFamilyName, MatchType::Exact, p1Updated.PackageFamilyNames[0]));
    REQUIRE(GetSearchedIds(combined, family) == std::set<std::string>{ p1Updated.Id });
}

// D5. The normalized name and publisher tables are populated as a side effect of the package name,
// so a rename has to move them. Nothing asserted on them before.
TEST_CASE("SQLiteIndex_Delta_SystemReference_NormalizedNameFollowsRename", "[sqliteindex][V2_1][delta]")
{
    auto original = MakePackage("Publisher1.Id", "Original Name");
    auto renamed = MakePackage(original.Id, "Replacement Name");

    DeltaTestContext context{ { original } };

    context.Update(renamed);
    context.GenerateDelta();

    Connection merged = context.OpenMergedConnection();

    auto names = GetSystemReferenceValues(merged, "norm_names2", "norm_name", renamed.Id);
    REQUIRE(names.size() == 1);

    // The old name must be gone rather than merely joined by the new one.
    for (const auto& name : names)
    {
        REQUIRE(name.find("original") == std::string::npos);
    }

    // Rather than predicting what normalization produces, require that the merged form holds
    // exactly what a full index built from the same data holds.
    Connection reference = Connection::Create(context.WorkingFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);

    REQUIRE(GetSystemReferenceValues(merged, "norm_names2", "norm_name", renamed.Id) ==
        GetSystemReferenceValues(reference, "norm_names2", "norm_name", renamed.Id));

    REQUIRE(GetSystemReferenceValues(merged, "norm_publishers2", "norm_publisher", renamed.Id) ==
        GetSystemReferenceValues(reference, "norm_publishers2", "norm_publisher", renamed.Id));
}

TEST_CASE("SQLiteIndex_Delta_SystemReference_RemovedPackageValuesAreInvisible", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "t1" }, { "c1" }, { "Family1_8wekyb3d8bbwe" }, { "PC-1" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2", { "t1" }, { "c1" }, { "Family2_8wekyb3d8bbwe" }, { "PC-2" });

    DeltaTestContext context{ { p1, p2 } };

    context.Remove(p2);
    context.GenerateDelta();

    {
        Connection delta = context.OpenDeltaConnection();

        // F3. A removal is recorded once, against the package. There are no per value tombstones,
        // so the package row is the only thing that can suppress these.
        REQUIRE(GetRowCount(delta, Delta::GetTableName("productcodes2")) == 0);
        REQUIRE(GetRowCount(delta, Delta::GetTableName("pfns2")) == 0);
    }

    Connection merged = context.OpenMergedConnection();

    REQUIRE(GetSystemReferenceValues(merged, "productcodes2", "productcode", p2.Id).empty());
    REQUIRE(GetSystemReferenceValues(merged, "pfns2", "pfn", p2.Id).empty());
    REQUIRE(GetSystemReferenceValues(merged, "productcodes2", "productcode", p1.Id) == std::set<std::string>{ "pc-1" });

    SQLiteIndex combined = context.OpenCombined();

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, p2.ProductCodes[0]));
    REQUIRE(combined.Search(request).Matches.empty());
}

// ---------------------------------------------------------------------------------------------
// Group E - generation of the one to many tables
// ---------------------------------------------------------------------------------------------

// E1. A package that trades one tag for another while keeping a third. Suppressing the baseline
// at the level of the package would lose the kept tag, because the delta never mentions it.
TEST_CASE("SQLiteIndex_Delta_OneToMany_AssociationsAreSuppressedPerRow", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "keep", "drop", "also_keep" }, { "cmd_keep", "cmd_drop" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2", { "other" }, { "other_cmd" });
    auto p1Updated = MakePackage(p1.Id, p1.Name, { "keep", "added", "also_keep" }, { "cmd_keep", "cmd_added" });

    DeltaTestContext context{ { p1, p2 } };

    context.Update(p1Updated);
    context.Remove(p2);

    context.GenerateDelta();

    Connection merged = context.OpenMergedConnection();

    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p1Updated.Id) == ToStringSet(p1Updated.Tags));

    // E2. The same defect on the other one to many table.
    REQUIRE(GetOneToManyValues(merged, "commands2", "command", p1Updated.Id) == ToStringSet(p1Updated.Commands));

    // F3 again, for the map tables.
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p2.Id).empty());
    REQUIRE(GetOneToManyValues(merged, "commands2", "command", p2.Id).empty());
}

// E3/E4. A value the baseline already knows is referenced at its existing rowid rather than copied,
// and a genuinely new value is numbered above everything the baseline holds.
TEST_CASE("SQLiteIndex_Delta_OneToMany_ValueRowIdAllocation", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "shared", "only1" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2", { "other" });
    auto p2Updated = MakePackage(p2.Id, p2.Name, { "other", "shared", "brand_new" });

    DeltaTestContext context{ { p1, p2 } };

    // Publisher2 gains a tag the baseline already has, plus one it does not.
    context.Update(p2Updated);
    context.GenerateDelta();

    rowid_t baselineMaxTagRowId = 0;
    rowid_t sharedRowIdInBaseline = 0;

    {
        Connection baseline = Connection::Create(context.BaselineFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);
        baselineMaxTagRowId = static_cast<rowid_t>(GetScalar(baseline, "SELECT MAX([rowid]) FROM [tags2]"));
        sharedRowIdInBaseline = static_cast<rowid_t>(GetScalar(baseline, "SELECT [rowid] FROM [tags2] WHERE [tag] = '" + p1.Tags[0] + "'"));
    }

    Connection delta = context.OpenDeltaConnection();

    // E4. The shared value is not copied into the delta; the map points at the baseline rowid.
    REQUIRE(GetStrings(delta, "SELECT [tag] FROM [delta_tags2]") == std::set<std::string>{ p2Updated.Tags[2] });

    auto mapTable = Delta::GetMapTableName("tags2");
    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [" + mapTable + "] WHERE [tag] = " + std::to_string(sharedRowIdInBaseline) + " AND [is_removed] = 0") == 1);

    // E3. The new value is numbered above the baseline, so the union cannot collide.
    REQUIRE(GetScalar(delta, "SELECT [rowid] FROM [delta_tags2] WHERE [tag] = '" + p2Updated.Tags[2] + "'") > baselineMaxTagRowId);

    // F4. Both sides resolve through the unioned value view.
    Connection merged = context.OpenMergedConnection();
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p2Updated.Id) == ToStringSet(p2Updated.Tags));
}

// E5. One new value shared by two packages is stored once and mapped twice.
TEST_CASE("SQLiteIndex_Delta_OneToMany_NewValueSharedByPackages", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "t1" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2", { "t2" });
    auto p1Updated = MakePackage(p1.Id, p1.Name, { "t1", "common_tag" });
    auto p2Updated = MakePackage(p2.Id, p2.Name, { "t2", "common_tag" });

    DeltaTestContext context{ { p1, p2 } };

    context.Update(p1Updated);
    context.Update(p2Updated);

    context.GenerateDelta();

    Connection delta = context.OpenDeltaConnection();

    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [delta_tags2] WHERE [tag] = '" + p1Updated.Tags[1] + "'") == 1);

    rowid_t valueRowId = static_cast<rowid_t>(GetScalar(delta, "SELECT [rowid] FROM [delta_tags2] WHERE [tag] = '" + p1Updated.Tags[1] + "'"));
    auto mapTable = Delta::GetMapTableName("tags2");
    REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [" + mapTable + "] WHERE [tag] = " + std::to_string(valueRowId) + " AND [is_removed] = 0") == 2);

    Connection merged = context.OpenMergedConnection();
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p1Updated.Id) == ToStringSet(p1Updated.Tags));
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p2Updated.Id) == ToStringSet(p2Updated.Tags));
}

// E6. Every value removed from a package. An empty current set must not read as "nothing changed".
TEST_CASE("SQLiteIndex_Delta_OneToMany_AllValuesRemoved", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "t1", "t2" }, { "c1" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2", { "t3" }, { "c2" });
    auto p1Updated = MakePackage(p1.Id, p1.Name, {}, {});

    DeltaTestContext context{ { p1, p2 } };

    context.Update(p1Updated);
    context.GenerateDelta();

    {
        Connection delta = context.OpenDeltaConnection();
        auto mapTable = Delta::GetMapTableName("tags2");
        REQUIRE(GetScalar(delta, "SELECT COUNT(*) FROM [" + mapTable + "] WHERE [is_removed] = 1") == 2);
    }

    Connection merged = context.OpenMergedConnection();
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p1Updated.Id).empty());
    REQUIRE(GetOneToManyValues(merged, "commands2", "command", p1Updated.Id).empty());

    // The other package is untouched.
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p2.Id) == ToStringSet(p2.Tags));
}

// E7. A baseline with no values at all, so the maximum rowid query has nothing to report.
TEST_CASE("SQLiteIndex_Delta_OneToMany_EmptyBaselineValueTable", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", {}, {});
    auto p1Updated = MakePackage(p1.Id, p1.Name, { "first" }, { "first_cmd" });

    DeltaTestContext context{ { p1 } };

    {
        Connection baseline = Connection::Create(context.BaselineFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);
        REQUIRE(GetRowCount(baseline, "tags2") == 0);
    }

    context.Update(p1Updated);
    REQUIRE_NOTHROW(context.GenerateDelta());

    Connection delta = context.OpenDeltaConnection();

    // Numbering has to start somewhere valid; rowid 0 is not.
    REQUIRE(GetScalar(delta, "SELECT [rowid] FROM [delta_tags2] WHERE [tag] = '" + p1Updated.Tags[0] + "'") > 0);

    Connection merged = context.OpenMergedConnection();
    REQUIRE(GetOneToManyValues(merged, "tags2", "tag", p1Updated.Id) == ToStringSet(p1Updated.Tags));
}

// ---------------------------------------------------------------------------------------------
// Group F - merged views
// ---------------------------------------------------------------------------------------------

// F5. In delta read mode the packages table is a view, so the check that decides whether an index
// has been prepared cannot rely on the table existing. Without this the 1.7 internal interface
// would be built over the views.
TEST_CASE("SQLiteIndex_Delta_CombinedIndexIsInPreparedState", "[sqliteindex][V2_1][delta]")
{
    DeltaTestContext context{ { MakePackage("Publisher1.Id", "Package 1") } };

    context.Add(MakePackage("Publisher2.Id", "Package 2"));
    context.GenerateDelta();

    SQLiteIndex combined = context.OpenCombined();

    // A prepared index reports no manifest paths; reaching the 1.7 interface would change that,
    // and would more likely fail outright looking for tables that are not there.
    auto results = combined.Search({});
    REQUIRE(results.Matches.size() == 2);

    REQUIRE(combined.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::Id).has_value());
}

// ---------------------------------------------------------------------------------------------
// Group G - combined open, affinity, and the negative paths
// ---------------------------------------------------------------------------------------------

TEST_CASE("SQLiteIndex_Delta_OpenWithBaseline_Search", "[sqliteindex][V2_1][delta]")
{
    // Immutable is the disposition the shipped path uses, and it is the one that reaches SQLite as
    // a URI rather than a plain path, so both files have to be named that way for the attach to
    // resolve at all.
    auto disposition = GENERATE(SQLiteStorageBase::OpenDisposition::Read, SQLiteStorageBase::OpenDisposition::Immutable);
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");

    DeltaTestContext context{ { p1 } };

    context.Add(p2);
    context.GenerateDelta();

    SQLiteIndex combined = context.OpenCombined(disposition);
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id, p2.Id });
}

TEST_CASE("SQLiteIndex_Delta_OpenWithBaseline_RemovedPackageExcluded", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");

    DeltaTestContext context{ { p1, p2 } };

    context.Remove(p2);
    context.GenerateDelta();

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id });
}

TEST_CASE("SQLiteIndex_Delta_UnmarkedBaselineRejected", "[sqliteindex][V2_1][delta]")
{
    DeltaTestContext context;
    context.CreateWorking({ MakePackage("Publisher1.Id", "Package 1") });
    context.CaptureBaseline(false);

    context.Add(MakePackage("Publisher2.Id", "Package 2"));

    // Without a designation the baseline has no identity, so nothing could tie the delta to it.
    REQUIRE_THROWS_HR(context.GenerateDelta(), APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED);
}

TEST_CASE("SQLiteIndex_Delta_MismatchedBaselineRejected", "[sqliteindex][V2_1][delta]")
{
    DeltaTestContext context{ { MakePackage("Publisher1.Id", "Package 1") } };

    // A second baseline with byte identical contents. Designating each gives it its own identity,
    // which is the point: a delta is tied to the baseline it was computed from, not to data that
    // happens to look like it.
    TempFile otherBaselineFile{ "delta_baseline_other"s, ".db"s };
    std::filesystem::copy_file(context.WorkingFile.GetPath(), otherBaselineFile.GetPath(), std::filesystem::copy_options::overwrite_existing);

    {
        SQLiteIndex other = SQLiteIndex::Open(otherBaselineFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
        other.PrepareForPackaging();
        other.MarkAsBaseline();
    }

    context.Add(MakePackage("Publisher2.Id", "Package 2"));
    context.GenerateDelta();

    REQUIRE_NOTHROW(context.OpenCombined());

    REQUIRE_THROWS_HR(
        SQLiteIndex::OpenWithBaseline(context.DeltaFile.GetPath().u8string(), otherBaselineFile.GetPath().u8string()),
        APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED);
}

// G9. Designating an index a second time mints a new identity, which invalidates any delta made
// against the first. Designation is deliberately not idempotent.
TEST_CASE("SQLiteIndex_Delta_ReMarkingBaselineInvalidatesDelta", "[sqliteindex][V2_1][delta]")
{
    DeltaTestContext context{ { MakePackage("Publisher1.Id", "Package 1") } };

    context.Add(MakePackage("Publisher2.Id", "Package 2"));
    context.GenerateDelta();

    REQUIRE_NOTHROW(context.OpenCombined());

    {
        SQLiteIndex baseline = SQLiteIndex::Open(context.BaselineFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
        baseline.MarkAsBaseline();
    }

    REQUIRE_THROWS_HR(context.OpenCombined(), APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED);
}

// G5. The combined form is a set of views over a union, so there is nothing to write back to.
TEST_CASE("SQLiteIndex_Delta_OpenWithBaseline_ReadWriteRejected", "[sqliteindex][V2_1][delta]")
{
    DeltaTestContext context{ { MakePackage("Publisher1.Id", "Package 1") } };

    context.Add(MakePackage("Publisher2.Id", "Package 2"));
    context.GenerateDelta();

    REQUIRE_THROWS_HR(context.OpenCombined(SQLiteStorageBase::OpenDisposition::ReadWrite), E_INVALIDARG);
}

// G6. A baseline that cannot be read at all has to fail rather than silently produce a delta only
// view of the world.
TEST_CASE("SQLiteIndex_Delta_OpenWithBaseline_MissingBaseline", "[sqliteindex][V2_1][delta]")
{
    DeltaTestContext context{ { MakePackage("Publisher1.Id", "Package 1") } };

    context.Add(MakePackage("Publisher2.Id", "Package 2"));
    context.GenerateDelta();

    TempFile missing{ "delta_missing_baseline"s, ".db"s };
    std::filesystem::remove(missing.GetPath());

    REQUIRE_THROWS(SQLiteIndex::OpenWithBaseline(context.DeltaFile.GetPath().u8string(), missing.GetPath().u8string()));
}

// G7/G8. The delta entry points are 2.1 only, and the base implementations say so rather than
// doing something undefined.
TEST_CASE("SQLiteIndex_Delta_NotSupportedBefore_2_1", "[sqliteindex][V2_0][delta]")
{
    TempFile indexFile{ "delta_unsupported"s, ".db"s };

    ManifestAndPath m1;
    CreateFakeManifestAndPath(m1, "Publisher1", "1.0");

    SQLiteIndex index = SQLiteIndex::CreateNew(indexFile, SQLiteVersion{ 2, 0 });
    index.AddManifest(m1.Manifest, m1.Path);
    index.PrepareForPackaging();

    REQUIRE_THROWS_HR(index.MarkAsBaseline(), HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));

    // G7. A 2.0 file in the delta position is nonsense, and the interface is what says so. The
    // baseline is never touched, so it does not need to exist.
    TempFile baselineFile{ "delta_unsupported_baseline"s, ".db"s };

    REQUIRE_THROWS_HR(
        SQLiteIndex::OpenWithBaseline(indexFile.GetPath().u8string(), baselineFile.GetPath().u8string()),
        HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
}

// ---------------------------------------------------------------------------------------------
// Group I - consistency
// ---------------------------------------------------------------------------------------------

// I2/I3. A package recorded as removed while still present in the index is a real inconsistency,
// but a package that was removed and re-added is not: it legitimately has both a tombstone and a
// live row. Confusing the two turns a silent bug into a loud but wrong integrity failure.
TEST_CASE("SQLiteIndex_Delta_CheckConsistency_ReAddedPackageIsNotCorruption", "[sqliteindex][V2_1][update_tracking]")
{
    TempFile indexFile{ "update_tracking"s, ".db"s };

    ManifestAndPath m1;
    CreateFakeManifestAndPath(m1, "Publisher1", "1.0");
    ManifestAndPath m2;
    CreateFakeManifestAndPath(m2, "Publisher2", "1.0");
    ManifestAndPath m3;
    CreateFakeManifestAndPath(m3, "Publisher3", "1.0");

    SQLiteIndex index = SQLiteIndex::CreateNew(indexFile, s_DeltaVersion);
    index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");
    index.AddManifest(m1.Manifest, m1.Path);
    index.AddManifest(m2.Manifest, m2.Path);
    index.AddManifest(m3.Manifest, m3.Path);

    // Removing the middle package and re-adding it leaves a tombstone beside a live row.
    index.RemoveManifest(m2.Manifest, m2.Path);
    index.AddManifest(m2.Manifest, m2.Path);

    REQUIRE(index.CheckConsistency(true));

    Connection connection = Connection::Create(indexFile, Connection::OpenDisposition::ReadOnly);
    REQUIRE(GetScalar(connection, "SELECT COUNT(*) FROM [update_tracking] WHERE [package] = '" + m2.Manifest.Id + "'") == 2);

    // The removal is still reported, since the old rowid genuinely was vacated.
    auto removals = Tracking::GetRemovalsSince(connection, 0, Tracking::RemovalBehavior::Record);
    REQUIRE(removals.size() == 1);

    // And it is not also reported as an update under that identity being gone.
    auto updates = Tracking::GetUpdatesSince(connection, 0, Tracking::RemovalBehavior::Record);
    REQUIRE(std::count_if(updates.begin(), updates.end(), [&](const auto& u) { return u.PackageIdentifier == m2.Manifest.Id; }) == 1);
}

TEST_CASE("SQLiteIndex_Delta_CheckConsistency_OnCombinedIndex", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1", { "t1" }, { "c1" }, {}, { "PC-1" });
    auto p2 = MakePackage("Publisher2.Id", "Package 2", { "t2" }, { "c2" }, {}, { "PC-2" });
    auto p1Updated = MakePackage(p1.Id, p1.Name, { "t1", "t3" }, p1.Commands, p1.PackageFamilyNames, p1.ProductCodes);

    DeltaTestContext context{ { p1, p2 } };

    context.Update(p1Updated);
    context.Remove(p2);
    context.GenerateDelta();

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(combined.CheckConsistency(true));
}

// ---------------------------------------------------------------------------------------------
// Group H - equivalence
//
// Enumerating what could go wrong in a merge is unbounded; comparing against a full index built
// from the same data is not. The targeted cases above exist to localize what this detects.
// ---------------------------------------------------------------------------------------------

namespace
{
    // Compares a combined index against a full index built from the same data.
    void RequireEquivalent(const SQLiteIndex& combined, const SQLiteIndex& full)
    {
        // H1. A battery covering the fields that the merged views are responsible for.
        std::vector<SearchRequest> requests;

        requests.emplace_back();

        auto addFieldRequest = [&](PackageMatchField field, MatchType type, std::string value, std::string second = {})
        {
            SearchRequest request;
            if (second.empty())
            {
                request.Inclusions.emplace_back(PackageMatchFilter(field, type, std::move(value)));
            }
            else
            {
                request.Inclusions.emplace_back(PackageMatchFilter(field, type, std::move(value), std::move(second)));
            }
            requests.emplace_back(std::move(request));
        };

        for (MatchType type : { MatchType::Exact, MatchType::Substring, MatchType::StartsWith })
        {
            addFieldRequest(PackageMatchField::Id, type, "Equivalence");
            addFieldRequest(PackageMatchField::Name, type, "Package");
            addFieldRequest(PackageMatchField::Moniker, type, "moniker");
            addFieldRequest(PackageMatchField::Tag, type, "shared");
            addFieldRequest(PackageMatchField::Tag, type, "changed");
            addFieldRequest(PackageMatchField::Command, type, "cmd");
            addFieldRequest(PackageMatchField::ProductCode, type, "PC");
            addFieldRequest(PackageMatchField::PackageFamilyName, type, "Family");
        }

        {
            SearchRequest query;
            query.Query = RequestMatch(MatchType::Substring, "Package");
            requests.emplace_back(std::move(query));
        }

        addFieldRequest(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, "Package Untouched", "Publisher");
        addFieldRequest(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, "Package Replacement", "Publisher");
        addFieldRequest(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, "Package Original", "Publisher");

        size_t matchedRequests = 0;

        for (size_t i = 0; i < requests.size(); ++i)
        {
            INFO("request index " << i);

            // A field and match type combination that the index does not support is not a delta
            // concern, but the two forms still have to agree about it.
            std::optional<std::set<std::string>> combinedIds;
            std::optional<std::set<std::string>> fullIds;

            try
            {
                combinedIds = GetSearchedIds(combined, requests[i]);
            }
            catch (...) {}

            try
            {
                fullIds = GetSearchedIds(full, requests[i]);
            }
            catch (...) {}

            REQUIRE(combinedIds.has_value() == fullIds.has_value());
            REQUIRE(combinedIds == fullIds);

            if (fullIds && !fullIds->empty())
            {
                ++matchedRequests;
            }
        }

        // Agreement is only meaningful if the battery actually matched something. Without this the
        // group passes when every request throws or returns nothing, which is exactly what a defect
        // in the merged views could cause.
        REQUIRE(matchedRequests != 0);

        // H3. The identifiers agreeing is not enough; the rowids that name them have to agree too,
        // since that is what the merge is built on and what a caller carries around.
        std::map<std::string, SQLiteIndex::IdType> combinedPrimaryIds;
        for (const auto& match : combined.Search({}).Matches)
        {
            combinedPrimaryIds[combined.GetPropertyByPrimaryId(match.first, PackageVersionProperty::Id).value()] = match.first;
        }

        std::map<std::string, SQLiteIndex::IdType> fullPrimaryIds;
        for (const auto& match : full.Search({}).Matches)
        {
            fullPrimaryIds[full.GetPropertyByPrimaryId(match.first, PackageVersionProperty::Id).value()] = match.first;
        }

        REQUIRE(combinedPrimaryIds.size() == fullPrimaryIds.size());

        // H2 and H3 iterate this map, so an empty one would make both pass vacuously.
        REQUIRE(!combinedPrimaryIds.empty());

        for (const auto& [packageId, primaryId] : combinedPrimaryIds)
        {
            INFO(packageId);

            auto full_itr = fullPrimaryIds.find(packageId);
            REQUIRE(full_itr != fullPrimaryIds.end());
            REQUIRE(primaryId == full_itr->second);

            // H2. Found is not the same as correct.
            for (PackageVersionProperty property : {
                PackageVersionProperty::Id,
                PackageVersionProperty::Name,
                PackageVersionProperty::Moniker,
                PackageVersionProperty::Version })
            {
                REQUIRE(combined.GetPropertyByPrimaryId(primaryId, property) == full.GetPropertyByPrimaryId(full_itr->second, property));
            }

            for (PackageVersionMultiProperty property : {
                PackageVersionMultiProperty::Tag,
                PackageVersionMultiProperty::Command,
                PackageVersionMultiProperty::PackageFamilyName,
                PackageVersionMultiProperty::ProductCode,
                PackageVersionMultiProperty::UpgradeCode,
                PackageVersionMultiProperty::Name,
                PackageVersionMultiProperty::Publisher })
            {
                auto combinedValues = combined.GetMultiPropertyByPrimaryId(primaryId, property);
                auto fullValues = full.GetMultiPropertyByPrimaryId(full_itr->second, property);

                std::sort(combinedValues.begin(), combinedValues.end());
                std::sort(fullValues.begin(), fullValues.end());

                REQUIRE(combinedValues == fullValues);
            }
        }
    }
}

TEST_CASE("SQLiteIndex_Delta_EquivalenceWithFullIndex", "[sqliteindex][V2_1][delta]")
{
    // The mutation set covers every kind of change the delta has to describe: a package added, one
    // removed, one whose values change, one renamed, and one left entirely alone.
    auto untouched = MakePackage("Equivalence.Untouched", "Package Untouched", { "shared", "keep" }, { "cmd_keep" }, { "Family0_8wekyb3d8bbwe" }, { "PC-UNTOUCHED" });
    auto removed = MakePackage("Equivalence.Removed", "Package Removed", { "shared", "gone" }, { "cmd_gone" }, { "Family1_8wekyb3d8bbwe" }, { "PC-REMOVED" });
    auto retagged = MakePackage("Equivalence.Retagged", "Package Retagged", { "shared", "changed" }, { "cmd_old" }, { "Family2_8wekyb3d8bbwe" }, { "PC-OLD", "PC-BOTH" });
    auto renamed = MakePackage("Equivalence.Renamed", "Package Original", { "shared" }, { "cmd_keep" }, { "Family3_8wekyb3d8bbwe" }, { "PC-RENAMED" });
    auto roundTrip = MakePackage("Equivalence.RoundTrip", "Package RoundTrip", { "shared" }, { "cmd_keep" }, {}, { "PC-ROUND" });
    auto retaggedUpdated = MakePackage(retagged.Id, retagged.Name, { "shared", "changed_new" }, { "cmd_new" }, retagged.PackageFamilyNames, { "PC-NEW", "PC-BOTH" });
    auto renamedUpdated = MakePackage(renamed.Id, "Package Replacement", renamed.Tags, renamed.Commands, renamed.PackageFamilyNames, renamed.ProductCodes);
    auto added = MakePackage("Equivalence.Added", "Package Added", { "shared", "brand" }, { "cmd_added" }, { "Family4_8wekyb3d8bbwe" }, { "PC-ADDED" });

    DeltaTestContext context{ { untouched, removed, retagged, renamed, roundTrip } };

    context.Remove(removed);
    context.Update(retaggedUpdated);
    context.Update(renamedUpdated);
    context.Add(added);

    // A removal and re-add in the same window, which is where rowid identity is hardest.
    context.Remove(roundTrip);
    context.Add(roundTrip);

    context.GenerateDelta();

    SQLiteIndex combined = context.OpenCombined();
    SQLiteIndex full = context.OpenFullIndex();

    REQUIRE(GetSearchedIds(full) == std::set<std::string>{
        untouched.Id, retaggedUpdated.Id, renamedUpdated.Id, roundTrip.Id, added.Id });

    RequireEquivalent(combined, full);
}

// H5. The degenerate case, which a consumer must not have to special case.
TEST_CASE("SQLiteIndex_Delta_EquivalenceWithEmptyDelta", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Equivalence.One", "Package One", { "shared" }, { "cmd_keep" }, { "Family0_8wekyb3d8bbwe" }, { "PC-1" });
    auto p2 = MakePackage("Equivalence.Two", "Package Two", { "shared", "changed" }, { "cmd2" }, {}, { "PC-2" });

    DeltaTestContext context{ { p1, p2 } };

    context.OpenWorkingForChanges();
    context.GenerateDelta();

    SQLiteIndex combined = context.OpenCombined();
    SQLiteIndex full = context.OpenFullIndex();

    RequireEquivalent(combined, full);
}


// ---------------------------------------------------------------------------------------------
// Group K - the change sequence
//
// The window a delta describes is defined by a monotonic sequence rather than by the write time.
// These cases cover the boundary that a whole second time cannot express, and the one failure
// mode a sequence has that a time does not.
// ---------------------------------------------------------------------------------------------

// K1. Every write takes a new sequence, and a package written twice keeps only the later one.
// A sequence that did not advance on update would leave the second change outside any window
// opened after the first.
TEST_CASE("SQLiteIndex_Delta_ChangeSequenceAdvancesOnEveryWrite", "[sqliteindex][V2_1][update_tracking]")
{
    TempFile indexFile{ "change_seq"s, ".db"s };

    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p1Updated = MakePackage(p1.Id, "Package 1 Renamed");

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(indexFile, s_DeltaVersion);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");

        Manifest m1 = CreateManifest(p1);
        index.AddManifest(m1, p1.Path);

        Manifest m2 = CreateManifest(p2);
        index.AddManifest(m2, p2.Path);

        Manifest m1Updated = CreateManifest(p1Updated);
        REQUIRE(index.UpdateManifest(m1Updated, p1.Path));
    }

    Connection connection = Connection::Create(indexFile, Connection::OpenDisposition::ReadOnly);

    // Three writes, but only two rows: the update replaced the first package's sequence rather
    // than adding a row.
    REQUIRE(GetRowCount(connection, "update_tracking") == 2);
    REQUIRE(GetScalar(connection, "SELECT MAX([change_seq]) FROM [update_tracking]") == 3);

    int64_t first = GetScalar(connection, "SELECT [change_seq] FROM [update_tracking] WHERE [package] = '" + p1Updated.Id + "'");
    int64_t second = GetScalar(connection, "SELECT [change_seq] FROM [update_tracking] WHERE [package] = '" + p2.Id + "'");

    // The updated package is now the more recent of the two, having started as the older.
    REQUIRE(first == 3);
    REQUIRE(second == 2);
}

// K2. The boundary case that the time based window could not express at all: a baseline captured
// in the same instant as the data it holds. Under whole second times compared inclusively, the
// entire baseline fell inside its own delta window.
TEST_CASE("SQLiteIndex_Delta_BaselineCapturedImmediatelyExcludesItsOwnData", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");

    // No delay anywhere: creation, capture and generation all happen as fast as they can.
    DeltaTestContext context{ { p1, p2 } };
    context.GenerateDelta();

    {
        Connection baseline = Connection::Create(context.BaselineFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);

        // Both packages were written before the baseline was taken, so the baseline's own sequence
        // must already account for them.
        REQUIRE(MetadataTable::GetNamedValue<int64_t>(baseline, "deltaBaselineSequence") == 2);
    }

    Connection delta = context.OpenDeltaConnection();
    REQUIRE(GetRowCount(delta, "delta_packages") == 0);

    SQLiteIndex combined = context.OpenCombined();
    REQUIRE(GetSearchedIds(combined) == std::set<std::string>{ p1.Id, p2.Id });
}

// K3. A migrated table has no sequences, so every row backfills to the same value and the first
// one issued afterwards is above it. An index designated as a baseline at that moment records 0,
// and the exclusive window correctly reports nothing that preceded the migration.
TEST_CASE("SQLiteIndex_Delta_TrackingMigrationBackfillsChangeSequence", "[sqliteindex][V2_1][update_tracking]")
{
    TempFile indexFile{ "change_seq_migrate"s, ".db"s };

    ManifestAndPath m1;
    CreateFakeManifestAndPath(m1, "Publisher1", "1.0");
    ManifestAndPath m2;
    CreateFakeManifestAndPath(m2, "Publisher2", "1.0");
    ManifestAndPath m3;
    CreateFakeManifestAndPath(m3, "Publisher3", "1.0");

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(indexFile, SQLiteVersion{ 2, 0 });
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");
        index.AddManifest(m1.Manifest, m1.Path);
        index.AddManifest(m2.Manifest, m2.Path);
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(indexFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(index.MigrateTo(s_DeltaVersion));
        REQUIRE(index.CheckConsistency(true));
    }

    {
        Connection connection = Connection::Create(indexFile, Connection::OpenDisposition::ReadOnly);
        REQUIRE(GetScalar(connection, "SELECT COUNT(*) FROM [update_tracking] WHERE [change_seq] = 0") == 2);
    }

    // A write after the migration has to be distinguishable from everything that preceded it.
    {
        SQLiteIndex index = SQLiteIndex::Open(indexFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.AddManifest(m3.Manifest, m3.Path);
    }

    Connection connection = Connection::Create(indexFile, Connection::OpenDisposition::ReadOnly);
    REQUIRE(GetScalar(connection, "SELECT [change_seq] FROM [update_tracking] WHERE [package] = '" + m3.Manifest.Id + "'") == 1);
}

// K4. The one direction a sequence fails in that a time does not. If the working index is rebuilt,
// its counter restarts below the baseline's and the window is empty, which would produce a
// silently empty delta. Generation has to refuse instead.
TEST_CASE("SQLiteIndex_Delta_SequenceBelowBaselineIsRejected", "[sqliteindex][V2_1][delta]")
{
    auto p1 = MakePackage("Publisher1.Id", "Package 1");
    auto p2 = MakePackage("Publisher2.Id", "Package 2");
    auto p3 = MakePackage("Publisher3.Id", "Package 3");

    DeltaTestContext context{ { p1, p2, p3 } };

    {
        Connection baseline = Connection::Create(context.BaselineFile.GetPath().u8string(), Connection::OpenDisposition::ReadOnly);
        REQUIRE(MetadataTable::GetNamedValue<int64_t>(baseline, "deltaBaselineSequence") == 3);
    }

    // A rebuilt working index, holding the same data but having recorded far fewer changes.
    TempFile rebuiltFile{ "change_seq_rebuilt"s, ".db"s };
    TempFile rebuiltDeltaFile{ "change_seq_rebuilt_delta"s, ".db"s };

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(rebuiltFile, s_DeltaVersion);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "0");
        Manifest manifest = CreateManifest(p1);
        index.AddManifest(manifest, p1.Path);
    }

    SQLiteIndex rebuilt = SQLiteIndex::Open(rebuiltFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
    rebuilt.SetProperty(SQLiteIndex::Property::DeltaBaselineIndexPath, context.BaselineFile.GetPath().u8string());
    rebuilt.SetProperty(SQLiteIndex::Property::DeltaOutputPath, rebuiltDeltaFile.GetPath().u8string());

    REQUIRE_THROWS_HR(rebuilt.PrepareForPackaging(), APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED);
}
