// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/2_0/PackageUpdateTrackingTable.h"
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteVersion.h>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    // Defined on ISQLiteIndex, because it crosses from here out to the business logic and that is
    // the only header both sides share. It is named here without the Delta prefix that
    // disambiguates it there, since this namespace already supplies it.
    using BaselineLocator = ISQLiteIndex::DeltaBaselineLocator;

    // Reads what a delta records about its baseline.
    //
    // Returns nothing when the database is not a delta, or when it does not carry all three
    // values. A delta that cannot fully name its baseline is unusable, and reporting a partial
    // locator would only invite a caller to act on half of one.
    std::optional<BaselineLocator> ReadBaselineLocator(const SQLite::Connection& connection);

    // Reads the identifier that designates a database as a baseline, if it carries one.
    //
    // Returns nothing for any index that has not been designated, which is every index that was
    // not prepared with DeltaMarkAsBaseline. Such an index cannot be paired with a delta.
    std::optional<std::string> ReadBaselineIdentifier(const SQLite::Connection& connection);

    // Writes a delta database describing the difference between a baseline index and the index
    // that is currently being packaged.
    //
    // This must run while the index being packaged still holds its update tracking table, as that
    // is the only record of which packages have changed since the baseline was produced.
    //
    // The source and the baseline assign the same rowid to a given package, so a package that
    // exists in both is described by rows that carry its baseline rowid, and a package that is new
    // to the source carries a rowid that the baseline cannot have used.
    //
    // The version is recorded as the delta's own schema version, so that opening the delta selects
    // the interface that knows how to merge it with a baseline.
    //
    // Nothing appears at the output path until the delta is complete: it is built beside the
    // destination and moved into place only on success, so a failure partway through cannot leave
    // something that looks like a usable delta. An output path that already exists is refused.
    //
    // The two baseline values are the caller's half of the locator that the delta records: where
    // the baseline will be published and which version it is. The third, the baseline's identity,
    // is not the caller's to supply -- it is read from the baseline itself, which is also what
    // establishes that the baseline was designated as one at all.
    void Generate(
        const SQLite::Connection& sourceConnection,
        const SQLite::Connection& baselineConnection,
        const std::string& baselineRelativeSourcePath,
        const std::string& baselinePackageVersion,
        const std::filesystem::path& deltaOutputPath,
        const SQLite::Version& version,
        const std::vector<V2_0::PackageUpdateTrackingTable::PackageData>& changedPackages,
        const std::set<SQLite::rowid_t>& removedPackages);
}
